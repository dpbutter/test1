
#pragma once

#include "Algorithm.hh"
#include "properties/TableauBase.hh"

namespace cadabra {

	class young_project_tensor : public Algorithm {
		public:
			young_project_tensor(const Kernel&, Ex&, bool);

			virtual bool     can_apply(iterator);
			virtual result_t apply(iterator&);

			bool modulo_monoterm;
			combin::symmetriser<unsigned int> sym;
		private:
			const TableauBase     *tb;
		};

	}
