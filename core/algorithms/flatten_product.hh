
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class flatten_product : public Algorithm {
		public:
			flatten_product(const Kernel&, Ex&);

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

			bool make_consistent_only, is_diff;
		};
	}

