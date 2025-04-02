
#pragma once

#include "Props.hh"
#include "properties/Symmetric.hh"

namespace cadabra {

	class Metric : public Symmetric, virtual public property {
		public:
			Metric();
			virtual std::string name() const override;
			virtual bool        parse(Kernel&, keyval_t&) override;
			virtual void        validate(const Kernel&, const Ex&) const override;
			virtual void        latex(std::ostream&) const override;

			int signature;
		};

	}
