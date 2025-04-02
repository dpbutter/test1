
#pragma once

#include "properties/Matrix.hh"
#include "properties/AntiSymmetric.hh"

namespace cadabra {

	class GammaMatrix : public AntiSymmetric, public Matrix, virtual public property {
		public:
			virtual ~GammaMatrix();
			virtual std::string name() const override;
			virtual void        latex(std::ostream&) const override;
			virtual bool        parse(Kernel&, keyval_t& keyvals) override;
			virtual std::string unnamed_argument() const override
				{
				return "explicit";
				};
			Ex metric;
		};

	}

