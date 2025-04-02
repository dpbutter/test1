
#include "Exceptions.hh"
#include "properties/Trace.hh"

using namespace cadabra;

Trace::Trace()
	{
	}

Trace::~Trace()
	{
	}

std::string Trace::name() const
	{
	return "Trace";
	}

std::string Trace::unnamed_argument() const
	{
	return "object";
	}

bool Trace::parse(Kernel&, keyval_t& keyvals)
	{
	keyval_t::const_iterator kv=keyvals.find("object");
	if(kv!=keyvals.end())
		obj = kv->second;
	kv=keyvals.find("indices");
	if(kv!=keyvals.end()) {
		if(kv->second.is_string())
			index_set_name=*(kv->second.begin()->name);
		else
			throw ConsistencyException("Trace: 'indices' argument should be a string.");
		}
	return true;
	}

void Trace::validate(const Kernel&, const Ex& ) const
	{
	}

void Trace::latex(std::ostream& str) const
	{
	str << name();
	}
