/*

Cadabra: a field-theory motivated computer algebra system.
Copyright (C) 2001-2014  Kasper Peeters <kasper.peeters@phi-sci.com>

This program is free software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "Parser.hh"
#include "PreProcessor.hh"
#include "Symbols.hh"

#include <sstream>
#include <internal/uniconv.h>
#include <iostream>
#include <typeinfo>

// #define DEBUG 1

std::istream& operator>>(std::istream& str, cadabra::Parser& pa)
	{
	std::string inp;
	while(std::getline(str >> std::ws, inp)) {
		// FIXME: This should all have been done in the manipulator, but when we
		// read the default settings from a string the input here is more than
		// just one line.
		if(inp[inp.size()-1]=='.') inp=inp.substr(0,inp.size()-1);
		//		std::cout << "[" << inp << "]" << std::endl;

#ifdef DEBUG
		std::cerr << inp << std::endl;
#endif
		
		pa.string2tree(inp);
		}
	// Remove the expression head.
	pa.finalise();

	return str;
	}

using namespace cadabra;

//std::ostream& operator<<(std::ostream& str, Parser& pa)
//	{
//	Ex_output eo(pa.tree);
//	eo.print_infix(str, pa.tree.begin());
//	return str;
//	}

str_node::bracket_t Parser::is_closing_bracket(const char32_t& br) const
	{
	if(br==')')     return str_node::b_none;
	if(br==']')     return str_node::b_square;
	if(br=='}')     return str_node::b_none;
	if(br=='}'+128) return str_node::b_curly;
	if(br=='>'+128) return str_node::b_pointy;
	return str_node::b_no;
	}

str_node::bracket_t Parser::is_opening_bracket(const char32_t& br) const
	{
	if(br=='(')     return str_node::b_none;
	if(br=='[')     return str_node::b_square;
	if(br=='{')     return str_node::b_none;
	if(br=='{'+128) return str_node::b_curly;
	if(br=='<'+128) return str_node::b_pointy;
	return str_node::b_no;
	}

str_node::parent_rel_t Parser::is_link(const char32_t& ln) const
	{
	if(ln=='^') return str_node::p_super;
	if(ln=='_') return str_node::p_sub;
	if(ln=='$') return str_node::p_property;
	if(ln=='&') return str_node::p_exponent;
	return str_node::p_none;
	}

Parser::Parser()
	{
	tree = std::make_shared<Ex>();
	tree->set_head(str_node("\\expression", str_node::b_none, str_node::p_none));
	parts=tree->begin();
	}

Parser::Parser(std::shared_ptr<Ex> t)
	: tree(t)
	{
	tree->clear();
	tree->set_head(str_node("\\expression", str_node::b_none, str_node::p_none));
	parts=tree->begin();
	}

Parser::Parser(std::shared_ptr<Ex> t, const std::string& str)
	: tree(t)
	{
	tree->clear();
	tree->set_head(str_node("\\expression", str_node::b_none, str_node::p_none));
	parts=tree->begin();
	string2tree(str);
	finalise();
	}

void Parser::erase()
	{
	str.clear();
	tree->clear();
	tree->insert(tree->begin(), str_node("\\expression", str_node::b_none, str_node::p_none));
	parts=tree->begin();
	current_mode.clear();
	current_bracket.clear();
	current_parent_rel.clear();
	}

void Parser::remove_empty_nodes()
	{
	Ex::iterator it=tree->begin();
	while(it!=tree->end()) {
		if((*it->name).size()==0) {
			tree->flatten(it);
			it=tree->erase(it);
			}
		++it;
		}
	}

void Parser::finalise()
	{
	if(tree->is_valid(tree->begin())==false) return;

	if(*(tree->begin()->name)=="\\expression") {
		tree->flatten(tree->begin());
		tree->erase(tree->begin());
		}
	}

void Parser::advance(unsigned int& i)
	{
//	if(get_token(i)>128) ++i;
	++i;
	}

char32_t Parser::get_token(unsigned int i)
	{
	if(str[i]=='\\')
		if(is_opening_bracket(str[i+1])!=str_node::b_no || is_closing_bracket(str[i+1])!=str_node::b_no)
			return str[i+1]+128;
	return str[i];
	}

bool Parser::string2tree(const std::string& inp)
	{
	if(inp.size()==0 || inp[0]=='#' || inp[0]=='%')
		return true;

	// Run the preprocessor.
#ifdef DEBUG
	std::cout << "running preprocessor" << std::endl;
#endif
	std::stringstream ss(inp), ss2;
	preprocessor pp;
	ss >> pp;
#ifdef DEBUG
	std::cout << "running preprocessor done" << std::endl;
#endif
	ss2 << pp;
	std::string str8="  "+ss2.str()+"  "; // for lookahead

#ifdef DEBUG
	std::cout << "converting Greek unicode to TeX" << std::endl;
#endif
	for (auto const& c : cadabra::symbols::greekmap) {
		size_t pos1 = 0;
		size_t pos2;
		while ((pos2 = str8.find(c.second, pos1)) != std::string::npos) {
			str8.replace(pos2, c.second.length(), c.first);
			pos1 = pos2 + c.first.length();
			}
		}
#ifdef DEBUG
	std::cout << "converting to utf32" << std::endl;
#endif

	utf_converter conv;
	str=conv.to_utf32(str8);

#ifdef DEBUG
	std::cout << "converted to utf32" << std::endl;
	for(size_t i=0; i<inp.size(); ++i)
		std::cout << (int)inp[i] << " ";
	std::cout << std::endl;
	for(size_t i=0; i<str.size(); ++i)
		std::cout << (int)str[i] << " ";
	std::cout << std::endl;
#endif

	// Initialise the parser.
	unsigned int i=0;
	current_mode.push_back(m_initialgroup);
	current_bracket.push_back((*parts).fl.bracket);
	current_parent_rel.push_back((*parts).fl.parent_rel);
	std::u32string tmp;  // buffer for object name
	//	str_node ss;

	Ex::iterator current_parent=parts;

	while(i<str.size()) {
		if(current_mode.size()==0) {
			return false;
			}
		char32_t c=get_token(i);
#ifdef DEBUG		
		std::cerr << i << " " << (int)c << " " << (c>128 ? "\\":"")
					 << (char)(c<128 ? c:(c-128)) << " mode "
					 << static_cast<int>(current_mode.back()) << std::endl;
#endif
		switch(current_mode.back()) {
			case m_skipwhite:
				// std::cerr << "m_skipwhite" << " " << c << std::endl;
				if(c!=' ' && c!='\n') current_mode.pop_back();
				else                  advance(i);
				break;
			case m_name: {
				// std::cerr << "m_name" << " " << c << std::endl;
				if(is_opening_bracket(c)!=str_node::b_no ||
				      ( is_link(c)!=str_node::p_none && ( tmp.size()==0 ||
																		!((tmp.size()>3 && tmp[0]!='\\' && is_opening_bracket(get_token(i+1))==str_node::b_no) || tmp[0]=='@' || (tmp[0]=='\\' && tmp[1]=='@'))) )) {
					// Note: the funky look-ahead above is to handle expressions of the form
					// somethinglong_{m}, which should trigger a child node, and
					// somethinglong_m, which should *not* (anything with more than 3 characters,
					// not started with a '\', followed by an underscore, is assumed to be
					// a python function.
					current_parent=tree->append_child(current_parent,str_node(tmp,
					                                  current_bracket.back(),
					                                  current_parent_rel.back()));
					current_mode.push_back(m_findchildren);
					tmp.clear();
					break;
					}
				if(is_closing_bracket(c)!=str_node::b_no) {
					if(tmp.size()>0) {
						tree->append_child(current_parent,str_node(tmp,
						                   current_bracket.back(),
						                   current_parent_rel.back()));
						tmp.clear();
						}
					current_mode.pop_back();
					break;
					}
				if(tmp.size()>0) {
					if(c=='+' || c=='-' || c=='*' || c=='/' || c=='\\' || c==' ' || c=='\n') {
						tree->append_child(current_parent,str_node(tmp,
						                   current_bracket.back(),
						                   current_parent_rel.back()));
						tmp.clear();
						if(c==' ' || c=='\n')
							advance(i);
						current_mode.pop_back();
						break;
						}
					}
				if(c=='+' || c=='-' || c=='*' || c=='/') {
					tmp+=c;
					tree->append_child(current_parent,str_node(tmp,
					                   current_bracket.back(),
					                   current_parent_rel.back()));
					tmp.clear();
					advance(i);
					current_mode.pop_back();
					break;
					}
				assert(c!=' ' && c!='\n');
				tmp+=c;
				advance(i);
				if(c=='\"')
					current_mode.push_back(m_verbatim);
				break;
				}
			case m_findchildren: {
				// std::cerr << "m_findchildren" << " " << c << std::endl;
				str_node::parent_rel_t pr=is_link(c);
				if(pr!=str_node::p_none) {
					advance(i);
					int cc=get_token(i);
					str_node::bracket_t br=is_opening_bracket(cc);
					if(br!=str_node::b_no) {
						current_bracket.push_back(br);
						current_parent_rel.push_back(pr);
						current_mode.push_back(m_childgroup);
						advance(i);
						}
					else {
						current_bracket.push_back(str_node::b_none);
						current_parent_rel.push_back(pr);
						if(pr==str_node::p_property)
							current_mode.push_back(m_property);
						else {
							// A '^' or '_' immediately followed by text, without bracket.
							// std::cerr << tmp[0] << std::endl;
							current_mode.push_back(m_singlecharname);
							}
						}
					break;
					}
				else {
					str_node::bracket_t br=is_opening_bracket(c);
					if(br!=str_node::b_no) {
						current_bracket.push_back(br);
						current_parent_rel.push_back(str_node::p_none);
						current_mode.push_back(m_childgroup);
						advance(i);
						break;
						}
					else {
						current_mode.pop_back();
						current_mode.push_back(m_skipwhite);
						current_parent=tree->parent(current_parent);
						//				 		advance(i);
						break;
						}
					}
				break;
				}
			case m_singlecharname:
				// This is for names 'a' or '\aaa' that appear as (...)^a
				// or (...)^\aaa .

				// std::cerr << "m_singlecharname" << " " << c << std::endl;
				tmp+=c;
				if(c=='\\') {
					current_mode.pop_back();
					current_mode.push_back(m_backslashname);
					advance(i);
					break;
					}
				else {
					tree->append_child(current_parent,str_node(tmp,
					                   str_node::b_none, /* current_bracket.back(), */
					                   current_parent_rel.back()));
					advance(i);
					tmp.clear();
					current_mode.pop_back();
					current_bracket.pop_back();
					current_parent_rel.pop_back();
					break;
					}
			case m_property: // properties do not need brackets
				if(c==' ' || c=='\n') {
					current_mode.pop_back();
					current_bracket.pop_back();
					current_parent_rel.pop_back();
					current_parent=tree->parent(current_parent);
					}
				else if(is_opening_bracket(c)) {
					if(tmp.size()>0) {
						current_parent=tree->append_child(current_parent,str_node(tmp,
						                                  current_bracket.back(),
						                                  current_parent_rel.back()));
						}
					current_mode.push_back(m_childgroup);
					}
				advance(i);
				tmp+=c;
				break;
			case m_verbatim:
				if(c=='\"')
					current_mode.pop_back();
				tmp+=c;
				advance(i);
				break;
			case m_backslashname:
#ifdef DEBUG
				std::cerr << "m_backslashname" << " " << c << std::endl;
#endif
				if(c==' ' || c=='\n' || c=='\\' || is_link(c)!=str_node::p_none
				      || is_closing_bracket(c)!=str_node::b_no) {
					current_mode.pop_back();
					tree->append_child(current_parent,str_node(tmp,
					                   current_bracket.back(),
					                   current_parent_rel.back()));
					tmp.clear();
					break;
					}
				if(is_opening_bracket(c)!=str_node::b_no) {
					// This happens with e.g. `\partial_\theta{f}`. The `\theta`
					// is a backslashname, and should be terminated as soon as
					// the opening brace is found. If you want the whole thing
					// to go into the subscript you should use `\partial_{\theta{f}}`
					// and that will then not trigger backslashname (just ordinary name).
					current_mode.pop_back();
					tree->append_child(current_parent,str_node(tmp,
					                   current_bracket.back(),
					                   current_parent_rel.back()));
					tmp.clear();
					break;
					}
				tmp+=c;
				advance(i);
				if(c=='\"')
					current_mode.push_back(m_verbatim);
				break;
			case m_childgroup: {
#ifdef DEBUG
				std::cerr << "m_childgroup" << " " << c << std::endl;
#endif
				str_node::bracket_t cb = is_closing_bracket(c);
				if(cb!=str_node::b_no) {
					// std::cerr << "leaving group" << std::endl;
					current_mode.pop_back();
					if(current_bracket.back()!=cb)
						throw std::logic_error("Closing bracket not matching opening bracket, or spurious backslash.");
					current_bracket.pop_back();
					current_parent_rel.pop_back();
					advance(i);
					break;
					}
				else {
#ifdef DEBUG
					std::cerr << "skip white then find name" << std::endl;
#endif
					current_mode.push_back(m_name);
					current_mode.push_back(m_skipwhite);
					break;
					}
				break;
				}
			case m_initialgroup:
				current_mode.push_back(m_name);
				current_mode.push_back(m_skipwhite);
				break;
			}
		}
	return true;
	}

bool Parser::is_number(const std::u32string& str) const
	{
	for(unsigned int i=0; i<str.size(); ++i)
		if(!isdigit(str[i])) return false;
	return true;
	}

