
#pragma once

#include "properties/Accent.hh"
#include "properties/Distributable.hh"

namespace cadabra {

	class DiracBar : public Accent, public Distributable, virtual public property {
		public:
			virtual ~DiracBar();
			virtual std::string name() const;
		};

	}
