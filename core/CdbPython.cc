
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include "CdbPython.hh"
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include "Exceptions.hh"

#ifndef CDBPYTHON_NO_NOTEBOOK
#include "DataCell.hh"
#endif

std::string cadabra::escape_quotes(const std::string& line)
	{
	return "''"+line+"''";
	//	std::string ret=std::replace_all_copy(line, "'", "\\'");
	//	std::replace_all(ret, "\"", "\\\"");
	//	return ret;
	}

std::string cadabra::cdb2python(const std::string& in_name, bool display)
	{
	// Read the file into a string.
	std::ifstream ifs(in_name);
	std::stringstream buffer;
	buffer << ifs.rdbuf();

	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);

	std::ostringstream ofs;
	ofs << "# cadabra2 package, auto-compiled " << std::put_time(&tm, "%F %T") << '\n'
	    << "# Do not modify - changing the timestamp of this file may cause import errors\n"
	    << "# Original file location: " << in_name << '\n'
	    << "import cadabra2\n"
	    << "from cadabra2 import *\n"
		 << "from cadabra2_defaults import *\n"
	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
	    << "temp__all__ = dir() + ['temp__all__']\n\n"
	    << "def display(ex):\n"
	    << "   pass\n\n";

	ofs << cdb2python_string(buffer.str(), display);

	ofs << '\n'
	    << "del locals()['display']\n\n"
	    << "try:\n"
	    << "    __all__\n"
	    << "except NameError:\n"
	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";

	return ofs.str();
	}

std::string cadabra::cdb2python_string(const std::string& blk, bool display)
	{
	std::stringstream str(blk);
	std::string line;
	std::string tmpblk;
	std::vector<std::string> newblks;
	ConvertData cv;

	std::pair<std::string, std::string> res;

	// We collect lines into blocks until they pass python
	// validation. If we ever hit an indentation error, we add
	// previous successful blocks and re-try, successively,
	// until the block compiles again.
	
	while(std::getline(str, line, '\n')) {
		res = cadabra::convert_line(line, cv, display);

//		if(res.second!="::empty")
//			tmpblk += res.first + res.second + "\n";

		if(res.second!="::empty")
			tmpblk += res.second;

		while(true) {
			// std::cerr << "CHECK:---\n" << tmpblk << "\n---" << std::endl;
			std::string error;
			int ic = is_python_code_complete(tmpblk, error);
			if(ic==1) { // complete
				newblks.push_back(res.first + tmpblk + "\n");
				tmpblk = "";
				break;
				}
			if(ic==0) { // incomplete
				tmpblk += "\n";
				break;
				}
			if(ic==-1) { // indentation error
				if(newblks.size()==0)
					break;
				// Grow block by adding previously ok block,
				// then try compiling again.
				tmpblk = newblks.back() + tmpblk;
				newblks.pop_back();
				}
			if(ic==-2) { 
				// This is either a genuine syntax error, or one line
				// added too many (after a previous 'incomplete' result).
				// Remove the line from the block, then re-run that line
				// separately.
				if(tmpblk != "") {
					if(size_t pos = tmpblk.rfind('\n'); pos != std::string::npos) {
						newblks.push_back(tmpblk.substr(0, pos)+"\n");
						tmpblk = tmpblk.substr(pos+1);
						// re-run
						}
					else throw ParseException(error);
					}
				else throw ParseException(error);
				}
			}
		}

	// Collect.
	std::string newblk;
	for(const auto& blk: newblks)
		newblk += blk;

	// Add anything still left and cross fingers.
	newblk += tmpblk+"\n";
	
	return newblk;
	}

//  1: complete
//  0: incomplete
// -1: indentation error, need backtracking
// -2: syntax error

int cadabra::is_python_code_complete(const std::string& code, std::string& error)
	{
	// The following code prints None, <code object> and <code object>.
	// Make sure that the string you feed here does *not* include the
	// newline on the last line.
	//
	// 
   //  from codeop import *
   //  
   //  str1='''def fun():
   //     print("hello")'''
   //  str2='''def fun():
   //     print("hello")
   //  '''
   //  str3='''print("hello")'''
   //  
   //  print(compile_command(str1, "<string>", "single"))
   //  print(compile_command(str2, "<string>", "single"))
   //  print(compile_command(str3, "<string>", "single"))

   //	pybind11::scoped_interpreter guard{}; // Ensures the Python interpreter is initialized.
	
	// Import the 'codeop' module.
	pybind11::object codeop = pybind11::module_::import("codeop");
	pybind11::object compile_command = codeop.attr("compile_command");

	// std::cerr << "CHECK:\n" << code << "END" << std::endl;
	try {
		pybind11::object result = compile_command(code, "<string>", "single");
		if( result.is_none() ) return 0;
		else                   return 1;
		}
	catch (pybind11::error_already_set& e) {
//		std::cerr << "EXCEPTION: " << e.what() << std::endl;
		error=e.what();
		if (std::string(e.what()).find("unexpected EOF") != std::string::npos) {
			return -1;
			}
		if (std::string(e.what()).find("Indentation") != std::string::npos) {
			return -1;
			}
		if (std::string(e.what()).find("SyntaxError") != std::string::npos) {
			return -2;
			}
		
		throw;
		}
	}

cadabra::ConvertData::ConvertData()
	{
	}

cadabra::ConvertData::ConvertData(const std::string& lhs_, const std::string& rhs_,
											 const std::string& op_, const std::string& indent_)
	: lhs(lhs_), rhs(rhs_), op(op_), indent(indent_)
	{
	}

std::pair<std::string, std::string> cadabra::convert_line(const std::string& line, ConvertData& cv, bool display)
	{
	std::string ret, prefix;

	auto& lhs    = cv.lhs;
	auto& rhs    = cv.rhs;
	auto& op     = cv.op;
	auto& indent = cv.indent;

	std::regex imatch("([\\s]*)([^\\s].*[^\\s])([\\s]*)");
	std::cmatch mres;

	std::string indent_line, end_of_line, line_stripped;
	if(std::regex_match(line.c_str(), mres, imatch)) {
		indent_line=std::string(mres[1].first, mres[1].second);
		end_of_line=std::string(mres[3].first, mres[3].second);
		line_stripped=std::string(mres[2]);
		}
	else {
		indent_line="";
		end_of_line="\n";
		line_stripped=line;
		}

	if(line_stripped.size()==0) {
		return std::make_pair(prefix, "");
		}

	// Do not do anything with comment lines.
	if(line_stripped[0]=='#') return std::make_pair(prefix, line);

	// Bare ';' gets replaced with 'display(_)' but *only* if we have no
	// preceding lines which have not finished parsing.
	if(line_stripped==";" && lhs=="") {
		if(display)
			return std::make_pair(prefix, indent_line+"display(_)");
		else
			return std::make_pair(prefix, indent_line);
		}

	// 'lastchar' is either a Cadabra termination character, or empty.
	// 'line_stripped' will have that character stripped, if present.
	std::string lastchar = line_stripped.substr(line_stripped.size()-1,1);
	if(lastchar=="." || lastchar==";" || lastchar==":") {
		if(lhs!="") {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			rhs += " "+line_stripped;
			ret = indent + lhs + " = Ex(r'" + escape_quotes(rhs) + "')";
			if(op==":=") {
				if(ret[ret.size()-1]!=';')
					ret+=";";
				ret+=" _="+lhs;
				}
			if(lastchar!=".")
				if(display)
					ret = ret + "; display("+lhs+")";
			indent="";
			lhs="";
			rhs="";
			op="";
			return std::make_pair(prefix, ret);
			}
		}
	else {
		// If we are a Cadabra continuation, add to the rhs without further processing
		// and return an empty line immediately.
		if(lhs!="") {
			rhs += line_stripped+" ";
			return std::make_pair(prefix, "::empty");
			}
		}

	// Add '__cdbkernel__' as first argument of post_process if it doesn't have that already.
	std::regex postprocmatch(R"(def post_process\(([^_]))");
	line_stripped = std::regex_replace(line_stripped, postprocmatch, "def post_process(__cdbkernel__, $1");

	// Replace $...$ with Ex(...).
	std::regex dollarmatch(R"(\$([^\$]*)\$)");
	line_stripped = std::regex_replace(line_stripped, dollarmatch, "Ex(r'''$1''', False)", std::regex_constants::match_default | std::regex_constants::format_default );

	// Replace 'converge(ex):' with 'server.progress('converge'); ex.reset(); while ex.changed(): server.progress(); server.end_progress();' properly indented.
	std::regex converge_match(R"(([ ]*)converge\(([^\)]*)\):)");
	std::smatch converge_res;
	if(std::regex_match(line_stripped, converge_res, converge_match)) {
		ret = indent_line+std::string(converge_res[1])+std::string(converge_res[2])+".reset(); _="+std::string(converge_res[2])+"\n"
		      + indent_line+std::string(converge_res[1])+"while "+std::string(converge_res[2])+".changed():";
		return std::make_pair(prefix, ret);
		}

	size_t found = line_stripped.find(":=");
	if(found!=std::string::npos) {
		// If the last character is not a Cadabra terminator, start a capture process.
		if(lastchar!="." && lastchar!=";" && lastchar!=":") {
			indent=indent_line;
			lhs=line_stripped.substr(0,found);
			rhs=line_stripped.substr(found+2);
			op=":=";
			return std::make_pair(prefix, "::empty");
			}
		else {
			line_stripped=line_stripped.substr(0,line_stripped.size()-1);
			ret = indent_line + line_stripped.substr(0,found) + " = Ex(r'"
			      + escape_quotes(line_stripped.substr(found+2)) + "')";
			std::string objname = line_stripped.substr(0,found);
			ret = ret + "; _="+objname;
			if(lastchar==";" && indent_line.size()==0 && display)
				ret = ret + "; display("+objname+")";
			}
		}
	else {   // {a,b,c}::Indices(real, parent=holo);
		found = line_stripped.find("::");
		if(found!=std::string::npos) {
			std::regex amatch(R"(([a-zA-Z]+)(\(.*\))?[ ]*[;\.:]*)");
			std::smatch ares;
			std::string subline=line_stripped.substr(found+2); // need to store the copy, not feed it straight into regex_match!
			if(std::regex_match(subline, ares, amatch)) {
				auto propname = std::string(ares[1]);
				auto argument = std::string(ares[2]);
				if(argument.size()>0) { // declaration with arguments
					argument=argument.substr(1,argument.size()-2);
					ret = indent_line + "__cdbtmp__ = "+propname
					      +"(Ex(r'"+escape_quotes(line_stripped.substr(0,found))
					      +"'), Ex(r'" +escape_quotes(argument) + "') )";
					}
				else {
					// no arguments
					line_stripped=line_stripped.substr(0,line_stripped.size()-1);
					ret = indent_line + "__cdbtmp__ = " + line_stripped.substr(found+2)
					      + "(Ex(r'"+escape_quotes(line_stripped.substr(0,found))+"'), Ex(r''))";
					}
				if(lastchar==";" && display)
					ret += "; display(__cdbtmp__)";
				}
			else {
				// inconsistent, who knows what will happen...
				ret = line; // inconsistent; you are asking for trouble.
				}
			}
		else {
			if(lastchar==";" && display) {
				prefix = "_ = " + prefix;
				ret = indent_line + line_stripped + " display(_)";
				}
			else {
				ret = indent_line + line_stripped;
				}
			}
		}
	return std::make_pair(prefix, ret+end_of_line);
	}

#ifndef CDBPYTHON_NO_NOTEBOOK

std::string cadabra::cnb2python(const std::string& in_name, bool for_standalone)
	{
	// Read the file into a Json object and get the cells. We go through
	// a proper notebook read because that way we can benefit from automatic
	// Jupyter -> Cadabra conversion.
	std::ifstream ifs(in_name);
	std::string content, line;
	while(std::getline(ifs, line))
		content+=line;
	cadabra::DTree doc;
	cadabra::JSON_deserialise(content, doc);

	std::time_t t = std::time(nullptr);
	std::tm tm = *std::localtime(&t);

	// Loop over input cells, compile them and write to python file
	std::ostringstream ofs;
	ofs << "# cadabra2 package, auto-compiled " << std::put_time(&tm, "%F %T") << '\n'
	    << "# Do not modify - changing the timestamp of this file may cause import errors\n"
	    << "# Original file location: " << in_name << '\n'
	    << "import cadabra2\n"
	    << "from cadabra2 import *\n"
		 << "from cadabra2_defaults import *\n"
	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
	    << "temp__all__ = dir() + ['temp__all__']\n\n"
	    << "def display(ex):\n"
	    << "   pass\n\n";


	// FIXME: this only does a single layer of cells below the top-level
	// 'document' cell; need recursing, in principle.
	auto docit=doc.begin();
	for(auto cell=doc.begin(docit); cell!=doc.end(docit); ++cell) {
		bool ioi = cell->ignore_on_import;
		if(for_standalone || !ioi) {
			if(cell->cell_type==cadabra::DataCell::CellType::python) {
				std::stringstream s, temp;
				s << cell->textbuf; // cell["source"].asString();
				ConvertData cv;
//				std::string line, lhs, rhs, op, indent;
				while (std::getline(s, line)) {
					std::pair<std::string, std::string> res
						= convert_line(line, cv, for_standalone); // lhs, rhs, op, indent, for_standalone);
					if(res.second!="::empty")
						ofs << res.second << '\n';
					}
				}
			}
		cell.skip_children();
		}
	// Ensure only symbols defined in this file get exported
	ofs << '\n'
	    << "del locals()['display']\n\n"
	    << "try:\n"
	    << "    __all__\n"
	    << "except NameError:\n"
	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";

	return ofs.str();
	}


// std::string cadabra::cnb2python(const std::string& name)
// 	{
// 	// Only compile if the notebook is newer than the compiled package
// 	struct stat f1, f2;
// 	if (stat(std::string(name + ".cnb").c_str(), &f1) == 0 && stat(std::string(name + ".py").c_str(), &f2) == 0) {
// 		if (f1.st_mtime < f2.st_mtime)
// 			return "";
// 		}
//
// 	// Read the file into a Json object and get the cells
// 	std::ifstream ifs(name + ".cnb");
// 	Json::Value nb;
// 	ifs >> nb;
// 	Json::Value& cells = nb["cells"];
//
// 	// Loop over input cells, compile them and write to python file
//
// 	std::ostringstream ofs;
// 	std::time_t t = std::time(nullptr);
// 	std::tm tm = *std::localtime(&t);
// 	ofs << "# cadabra2 package, auto-compiled " << std::put_time(&tm, "%F %T") << '\n'
// 	    << "import cadabra2\n"
// 	    << "import imp\n"
// 	    << "from cadabra2 import *\n"
// 	    << "__cdbkernel__ = cadabra2.__cdbkernel__\n"
// 	    << "temp__all__ = dir() + ['temp__all__']\n\n"
// 	    << "def display(ex):\n"
// 	    << "   pass\n\n";
//
// //	    << "with open(imp.find_module('cadabra2_defaults')[1]) as f:\n"
// //	    << "   code = compile(f.read(), 'cadabra2_defaults.py', 'exec')\n"
// //	    << "   exec(code)\n\n";
//
// 	for (auto cell : cells) {
// 		if (cell["cell_type"] == "input") {
// 			std::stringstream s, temp;
// 			s << cell["source"].asString();
// 			std::string line, lhs, rhs, op, indent;
// 			while (std::getline(s, line))
// 				ofs << convert_line(line, lhs, rhs, op, indent) << '\n';
// 		}
// 	}
// 	// Ensure only symbols defined in this file get exported
// 	ofs << '\n'
// 	    << "del locals()['display']\n\n"
// 	    << "try:\n"
// 	    << "    __all__\n"
// 	    << "except NameError:\n"
// 	    << "    __all__  = list(set(dir()) - set(temp__all__))\n";
//
// 	return ofs.str();
// 	}
//

#endif
