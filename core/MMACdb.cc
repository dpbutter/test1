

#include "Config.hh"
#include <boost/python.hpp>
#include "Functional.hh"
#include "MMACdb.hh"
#include "PreClean.hh"
#include "Cleanup.hh"
#include "Parser.hh"
#include "Kernel.hh"
#include "DisplayMMA.hh"
#include "algorithms/substitute.hh"

using namespace cadabra;

WSLINK        MMA::lp = 0;
WSEnvironment MMA::stdenv = 0;

//#define DEBUG 1

Ex::iterator MMA::apply_mma(const Kernel& kernel, Ex& ex, Ex::iterator& it, const std::vector<std::string>& wrap,
                            std::vector<std::string> args, const std::string& method)
	{
	// We first need to print the sub-expression using DisplaySympy,
	// optionally with the head wrapped around it and the args added
	// (if present).
	std::ostringstream str;

	for(size_t i=0; i<wrap.size(); ++i) {
		str << wrap[i] << "[";
		}

	DisplayMMA ds(kernel, ex, true);
	ds.output(str, it);

	if(wrap.size()>0)
		if(args.size()>0)
			for(size_t i=0; i<args.size(); ++i)
				str << ", " << args[i];
	for(size_t i=1; i<wrap.size(); ++i)
		str << "]";
	str << method;

	if(wrap.size()>0)
		str << "]";

	// We then execute the expression in Python.

	//ex.print_recursive_treeform(std::cerr, it);
#ifdef DEBUG
	std::cerr << "feeding " << str.str() << std::endl;
#endif

	std::string result;

	// ---------------------
	setup_link();

	WSPutFunction(lp, "EvaluatePacket", 1L);
	WSPutFunction(lp, "ToString", 1L);
	WSPutFunction(lp, "FullForm", 1L);
	WSPutFunction(lp, "ToExpression", 1L);
	WSPutUTF8String(lp, (const unsigned char *)str.str().c_str(), str.str().size());
	WSEndPacket(lp);
	WSFlush(lp);

	// std::cerr << "flushed" << std::endl;

	int pkt=0;
	while( (pkt = WSNextPacket(lp), pkt) && pkt != RETURNPKT ) {
		// std::cerr << "received packet " << pkt << std::endl;
		WSNewPacket(lp);
		if (WSError(lp)) {
			std::cerr << "error" << std::endl;
			}
		}
	// std::cerr << "packet now " << pkt << std::endl;

	const char *out;
	if(! WSGetString(lp, &out)) {
		throw InternalError("Unable to read from WSTP link");
		}
	else {
		result=out;
		WSReleaseString(lp, out);
		if(result=="$Failed") {
			throw RuntimeException("Mathematica failed to handle expression.");
			}
		}
	// -------------------------------

	// After that, we construct a new sub-expression from this string by using our
	// own parser, and replace the original.

#ifdef DEBUG
	std::cerr << "result: " << result << std::endl;
#endif

	result = ds.preparse_import(result);

#ifdef DEBUG
	std::cerr << "preparsed: " << result << std::endl;
#endif

	auto ptr = std::make_shared<Ex>();
	cadabra::Parser parser(ptr);
	std::stringstream istr(result);
	istr >> parser;

	pre_clean_dispatch_deep(kernel, *parser.tree);
	cleanup_dispatch_deep(kernel, *parser.tree);

	//parser.tree->print_recursive_treeform(std::cerr, parser.tree->begin());

#ifdef DEBUG
	std::cerr << "cleaned up: " << *parser.tree << std::endl;
#endif
	
	ds.import(*parser.tree);

#ifdef DEBUG
	std::cerr << "imported: " << *parser.tree << std::endl;
#endif

	pre_clean_dispatch_deep(kernel, *parser.tree);
	cleanup_dispatch_deep(kernel, *parser.tree);

	Ex::iterator first=parser.tree->begin();
	// std::cerr << "reparsed " << Ex(first) << std::endl;
	it = ex.move_ontop(it, first);

	return it;
	}

void MMA::setup_link()
	{
	if(lp!=0) return; // already setup

	char argvi[4][512] = { "-linkname", Mathematica_KERNEL_EXECUTABLE " -mathlink", "-linkmode", "launch" };
	char *argv[4];
	for (size_t i=0; i<4; ++i)
		argv[i] = argvi[i];
	int  argc = 4;

	int errno;
	stdenv = WSInitialize((WSEnvironmentParameter)0);
	if(stdenv==0)
		throw InternalError("Failed to initialise WSTP");

	// std::cerr << "initialised" << std::endl;

	try {
		lp = WSOpenArgcArgv(stdenv, argc, argv, &errno);
		if(lp==0 || errno!=WSEOK) {
			// std::cerr << errno << ", " << WSErrorMessage(lp) << ";" << std::endl;
			lp=0;
			WSDeinitialize(stdenv);
			throw InternalError("Failed to open Mathematica link");
			}
		}
	catch(std::exception& ex) {
		lp=0;
		WSDeinitialize(stdenv);
		throw InternalError("Failed to open Mathematica link");
		}

	// std::cerr << "loopback link open" << std::endl;

	WSActivate(lp);
	}

void MMA::teardown_link()
	{
	if(lp!=0) {
		WSClose(lp);
		WSDeinitialize(stdenv);
		lp=0;
		}
	}
