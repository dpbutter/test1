
#pragma once

#include "Algorithm.hh"

namespace cadabra {

	class young_project_product : public Algorithm {
		public:
			young_project_product(const Kernel&, Ex&);

			virtual bool     can_apply(iterator) override;
			virtual result_t apply(iterator&) override;
		};

	}
