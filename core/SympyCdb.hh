
#pragma once

#include "Props.hh"
#include "Storage.hh"
#include "Kernel.hh"
#include "Stopwatch.hh"
#include "DisplaySympy.hh"

#ifndef NO_SYMPY
#include <pybind11/pybind11.h>
#endif

namespace sympy {

#ifndef NO_SYMPY
	/// Helper class to enable conversion from/to sympy.

	class SympyBridge : public cadabra::DisplaySympy {
		public:
			SympyBridge(const cadabra::Kernel&, std::shared_ptr<cadabra::Ex>);
			virtual ~SympyBridge();

			pybind11::object export_ex();
			void             import_ex(const std::string&);

		private:
			std::shared_ptr<cadabra::Ex> ex;
		};
#endif

	/// \ingroup scalar
	///
	/// Functionality to act with Sympy on all scalar parts of an expression, and
	/// keep the result in-place. This is a higher-level function than
	/// 'apply' below.

	//	cadabra::Ex* map_sympy(const cadabra::Kernel&, cadabra::Ex&,
	//								  const std::vector<std::string>& wrap, const std::string& args, const std::string& method);

	/// \ingroup scalar
	///
	/// Functionality to act with Sympy functions on (parts of) Cadabra Ex expressions
	/// and read the result back into the same Ex. This duplicates some of the
	/// logic in PythonCdb.hh, in particular make_Ex_from_string, but it is best to
	/// keep these two completely separate.

	cadabra::Ex::iterator apply(const cadabra::Kernel&, cadabra::Ex&, cadabra::Ex::iterator&,
	                            const std::vector<std::string>& wrap, std::vector<std::string> args, const std::string& method);

	//    /// \ingroup scalar
	//    ///
	//    /// Low-level function to feed a string to Python and read the result back in
	// 	/// as a Cadabra Ex. As compared to 'apply' above, this starts from a string rather
	// 	/// than an Ex, and hence gives more flexibility in constructing input.
	//
	// 	Ex python(Kernel&, Ex&, Ex::iterator&, const std::string& head, const std::string& args);


	/// \ingroup scalar
	///
	/// Use Sympy to invert a matrix, given a set of rules determining its
	/// sparse components. Will return a set of Cadabra rules for the
	/// inverse matrix.

	void invert_matrix(const cadabra::Kernel&, cadabra::Ex& ex, cadabra::Ex& rules, const cadabra::Ex& tocompute);

	/// \ingroup scalar
	///
	/// Use Sympy to compute the determinant of a matrix, given a set of rules determining
	/// its sparse components. Will add the rules to the list.

	void determinant(const cadabra::Kernel&, cadabra::Ex& ex, cadabra::Ex& rules, const cadabra::Ex& tocompute);
	void trace(const cadabra::Kernel&, cadabra::Ex& ex, cadabra::Ex& rules, const cadabra::Ex& tocompute);

	cadabra::Ex fill_matrix(const cadabra::Kernel&, cadabra::Ex& ex, cadabra::Ex& rules);
	//	extern Stopwatch sympy_stopwatch;

};
