
#pragma once

#include "properties/CommutingBehaviour.hh"

namespace cadabra {

	class AntiCommuting : virtual public CommutingBehaviour {
		public:
			virtual ~AntiCommuting();
			virtual std::string name() const;
			virtual int sign() const
				{
				return -1;
				}
		};

	}
