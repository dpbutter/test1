
#include "Functional.hh"

namespace cadabra {

	void do_list(const Ex& tr, Ex::iterator it, std::function<bool(Ex::iterator)> f)
		{
		if(it==tr.end()) return;

		if(*it->name=="\\comma") {
			Ex::sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				Ex::sibling_iterator nxt=sib;
				++nxt;
				if(f(sib)==false)
					return;
				sib=nxt;
				}
			}
		else {
			f(it);
			}
		}

	int list_size(const Ex& tr, Ex::iterator it)
		{
		if(*it->name=="\\comma")
			return tr.number_of_children(it);
		else
			return 1;
		}

	Ex::iterator find_in_subtree(const Ex& tr, Ex::iterator it, std::function<bool(Ex::iterator)> f, bool including_head)
		{
		if(it==tr.end()) return it;

		Ex::post_order_iterator walk=it, last=it;
		++last;
		walk.descend_all();

		do {
			auto nxt=walk;
			++nxt;

			if(f(walk))
				return walk;

			walk=nxt;

			if(including_head==false && walk==it)
				break;
			}
		while(walk!=last);

		return tr.end();
		}


	Ex::iterator find_in_list(const Ex& tr, Ex::iterator it, std::function<Ex::iterator(Ex::iterator)> f)
		{
		if(*it->name=="\\comma") {
			Ex::sibling_iterator sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				Ex::iterator ret = f(sib);
				if(ret!=tr.end())
					return ret;
				++sib;
				}
			return tr.end();
			}
		else {
			return f(it);
			}
		}

	Ex make_list(Ex el)
		{
		auto it=el.begin();

		if(*it->name!="\\comma")
			el.wrap(it, str_node("\\comma"));

		return el;
		}

	}

