
#pragma once

#include "Props.hh"
#include "properties/Derivative.hh"
#include "properties/DifferentialFormBase.hh"

namespace cadabra {

	class ExteriorDerivative : public Derivative, public DifferentialFormBase {
		public:
			virtual ~ExteriorDerivative();
			virtual std::string name() const override;

			virtual Ex degree(const Properties&, Ex::iterator) const override;
		};

	}
