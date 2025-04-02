
#pragma once

#include "Exceptions.hh"
#include "properties/WeightBase.hh"

namespace cadabra {

	class WeightInherit : virtual public WeightBase {
		public:
			virtual ~WeightInherit();
			
			// The following exception class is thrown when 'value' cannot figure out the
			// weight because a sum contains terms of different weight.
			class WeightException : public ConsistencyException {
				public:
					WeightException(const std::string&);
				};

			virtual bool          parse(Kernel&, std::shared_ptr<Ex>, keyval_t&) override;
			virtual multiplier_t  value(const Kernel&, Ex::iterator, const std::string& forcedlabel) const override;
			virtual std::string   unnamed_argument() const override
				{
				return "type";
				};
			virtual std::string   name() const override;

			enum CombinationType { multiplicative, additive, power } combination_type;

			multiplier_t value_self;
		};

	}
