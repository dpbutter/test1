
#include "PreClean.hh"
#include "Cleanup.hh"

namespace cadabra {

	void pre_clean_dispatch(const Kernel& kernel, Ex& ex, Ex::iterator& it)
		{
		if(*it->name!="1" && it->is_unsimplified_rational()) cleanup_rational(kernel, ex, it);

		if(*it->name=="\\frac")                      cleanup_frac(kernel, ex, it);
		else if(*it->name=="\\sub")                  cleanup_sub(kernel, ex, it);
		else if(*it->name=="\\sqrt")                 cleanup_sqrt(kernel, ex, it);
		else if(*it->name=="\\prod")                 cleanup_prod(kernel, ex, it);
		else if((*it->name).substr(0,2)=="UP" || (*it->name).substr(0,2)=="DN")  cleanup_updown(kernel, ex, it);

		cleanup_indexbracket(kernel, ex, it);
		}

	void pre_clean_dispatch_deep(const Kernel& k, Ex& tr)
		{
		return cleanup_dispatch_deep(k, tr, &pre_clean_dispatch);
		}

	void cleanup_updown(const Kernel&, Ex&, Ex::iterator& st)
		{
		std::string rn=*st->name;
		bool isup=true;
		if(rn.substr(0,2)=="DN")
			isup=false;

		rn=rn.substr(2);

		//	tr.flatten(st);
		//	st=tr.erase(st);
		if(isup) st->fl.parent_rel=str_node::p_super;
		else     st->fl.parent_rel=str_node::p_sub;
		st->name=name_set.insert(rn).first;
		}

	void cleanup_rational(const Kernel&, Ex&, Ex::iterator& st)
		{
		multiplier_t num(*st->name);
		num.canonicalize();
		st->name=name_set.insert("1").first;
		multiply(st->multiplier,num);
		}

	void cleanup_frac(const Kernel&, Ex& tr, Ex::iterator& st)
		{
		// Catch \frac{} nodes with one argument; those are supposed to be read as 1/(...).
		// The only exception is \frac{#}, which needs to stay as it is.
		if(tr.number_of_children(st)==1) {
			if(tr.begin(st)->is_range_wildcard()) return;
			tr.insert(tr.begin(st), str_node("1"));
			}

		// Turn this into a \prod node. Everything except the first child
		// should be wrapped in a \pow{..}{-1} node.

		auto sib=tr.begin(st);
		++sib;
		while(sib!=tr.end(st)) {
			sib = tr.wrap(sib, str_node("\\pow"));
			multiply( tr.append_child(sib, str_node("1"))->multiplier, -1 );
			++sib;
			}
		st->name=name_set.insert("\\prod").first;


		//	assert(tr.number_of_children(st)>1);
		//	Ex::sibling_iterator it=tr.begin(st);
		//	multiplier_t rat;
		//
		//	bool allnumerical=true;
		//	rat=*(it->multiplier);
		//	if(it->is_rational()==false)
		//		allnumerical=false;
		//
		//	one(it->multiplier);
		//	++it;
		//	while(it!=tr.end(st)) {
		//		if(*it->multiplier==0) {
		//			// CHECK: do these zeroes get handled correctly elsewhere?
		//			return;
		//			}
		//		rat/=*it->multiplier;
		//		one(it->multiplier);
		//		if(it->is_rational()==false) allnumerical=false;
		//		++it;
		//		}
		//	if(allnumerical) { // can remove the \frac altogether
		//		tr.erase_children(st);
		//		st->name=name_set.insert("1").first;
		//		}
		//	else { // just remove the all-numerical child nodes
		//		it=tr.begin(st);
		//		++it;
		//		while(it!=tr.end(st)) {
		//			if(it->is_rational())
		//				it=tr.erase(it);
		//			else ++it;
		//			}
		//		if(tr.number_of_children(st)==1) {
		//			tr.begin(st)->fl.bracket=st->fl.bracket;
		//			tr.begin(st)->fl.parent_rel=st->fl.parent_rel;
		//			multiply(tr.begin(st)->multiplier, *st->multiplier);
		//			tr.flatten(st);
		//			st=tr.erase(st);
		//			}
		//		}
		//	multiply(st->multiplier, rat);

		}

	void cleanup_sqrt(const Kernel&, Ex& tr, Ex::iterator& st)
		{
		st->name=name_set.insert("\\pow").first;
		multiply(tr.append_child(st, str_node("1"))->multiplier, multiplier_t(1)/2);
		}

	void cleanup_prod(const Kernel&, Ex& tr, Ex::iterator& st)
		{
		auto sib=tr.begin(st);
		while(sib!=tr.end(st)) {
			if(*sib->name=="\\")
				throw std::logic_error("Single backslash as name not allowed, did you mean to write a single slash but wrote two?");
			++sib;
			}
		}

	void cleanup_sub(const Kernel&, Ex& tr, Ex::iterator& it)
		{
		assert(tr.number_of_children(it)>1); // To guarantee that we have really cleaned up that old stuff.

		it->name=name_set.insert("\\sum").first;
		Ex::sibling_iterator sit=tr.begin(it);

		// Make sure that all terms have the right sign, and zeroes are removed.
		if(*sit->multiplier==0) sit=tr.erase(sit);
		else                    ++sit;

		while(sit!=tr.end(it)) {
			if(*sit->multiplier==0)
				sit=tr.erase(sit);
			else {
				flip_sign(sit->multiplier);
				++sit;
				}
			}

		// Single-term situation: remove the \sum.
		if(tr.number_of_children(it)==0) {
			zero(it->multiplier);
			it->name=name_set.insert("1").first;
			}
		else {
			if(tr.number_of_children(it)==1) {
				sit=tr.begin(it);
				sit->fl.parent_rel=it->fl.parent_rel;
				sit->fl.bracket=it->fl.bracket;
				multiply(sit->multiplier, *it->multiplier);
				tr.flatten(it);
				it=tr.erase(it);
				}
			}
		}

	void cleanup_indexbracket(const Kernel&, Ex& tr, Ex::iterator& it)
		{
		if((*it->name).size()==0) {
			auto sib=tr.begin(it);
			if(sib->fl.parent_rel!=str_node::p_super && sib->fl.parent_rel!=str_node::p_sub) {
				++sib;
				while(sib!=tr.end(it)) {
					if(sib->fl.parent_rel==str_node::p_super || sib->fl.parent_rel==str_node::p_sub) {
						it->name=name_set.insert("\\indexbracket").first;
						return;
						}
					++sib;
					}
				}
			}
		else if(*it->name=="\\prod" || *it->name=="\\sum") {
			auto sib=tr.begin(it);
			while(sib!=tr.end(it)) {
				if(sib->fl.parent_rel==str_node::p_super || sib->fl.parent_rel==str_node::p_sub) {
					auto ibrack=tr.insert(it,str_node("\\indexbracket"));
					Ex::sibling_iterator nxt=it;
					++nxt;
					tr.reparent(ibrack,Ex::sibling_iterator(it),nxt);
					it=tr.begin(ibrack);
					auto sib=tr.begin(it);
					while(sib!=tr.end(it)) {
						if(sib->fl.parent_rel==str_node::p_super || sib->fl.parent_rel==str_node::p_sub) {
							tr.append_child(ibrack,*sib);
							sib=tr.erase(sib);
							}
						else ++sib;
						}
					it=ibrack;
					return;
					}
				++sib;
				}
			}
		}

	std::string replace_all(std::string const& original, std::string const& from, std::string const& to )
		{
		std::string results;
		std::string::const_iterator end = original.end();
		std::string::const_iterator current = original.begin();
		std::string::const_iterator next = std::search( current, end, from.begin(), from.end() );
		while ( next != end ) {
			results.append( current, next );
			results.append( to );
			current = next + from.size();
			next = std::search( current, end, from.begin(), from.end() );
			}
		results.append( current, next );
		return results;
		}

	}
