
#include "properties/SelfCommuting.hh"

using namespace cadabra;

SelfCommuting::~SelfCommuting()
	{
	}

std::string SelfCommuting::name() const
	{
	return "SelfCommuting";
	}

int SelfCommuting::sign() const
	{
	return 1;
	}
