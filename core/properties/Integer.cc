
#include "properties/Integer.hh"
#include "Cleanup.hh"
#include "Kernel.hh"
#include "algorithms/collect_terms.hh"

using namespace cadabra;

std::string Integer::name() const
	{
	return "Integer";
	}

bool Integer::parse(Kernel& kernel, keyval_t& keyvals)
	{
	keyval_t::iterator kv=keyvals.find("range");
	if(kv!=keyvals.end()) {
		if(*kv->second.begin()->name!="\\sequence")
			throw ConsistencyException("Integer range should use '..' notation.");

		from=Ex(Ex::child(kv->second.begin(), 0));
		to  =Ex(Ex::child(kv->second.begin(), 1));

		Ex::iterator sm=difference.set_head(str_node("\\sum"));
		difference.append_child(sm, to.begin())->fl.bracket=str_node::b_round;
		Ex::iterator term2=difference.append_child(sm, from.begin());
		flip_sign(term2->multiplier);
		term2->fl.bracket=str_node::b_round;
		difference.append_child(sm, str_node("1"))->fl.bracket=str_node::b_round;

		Ex::sibling_iterator sib=difference.begin(sm);
		while(sib!=difference.end(sm)) {
			if(*sib->name=="\\sum") {
				difference.flatten(sib);
				sib=difference.erase(sib);
				}
			else ++sib;
			}

		collect_terms ct(kernel, difference);
		ct.apply(sm);
		}

	//	Ex::iterator top=difference.begin();
	//	cleanup_dispatch(kernel, difference, top);

	return true;
	}

void Integer::display(std::ostream& str) const
	{
	str << "Integer";
	if(from.begin()!=from.end()) {
		str << "(" << *(from.begin()->multiplier) << ".."
		    << *(to.begin()->multiplier) << ")";
		}
	}
