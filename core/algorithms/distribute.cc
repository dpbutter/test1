
#include "distribute.hh"
#include "Cleanup.hh"

#include "properties/Distributable.hh"

#include "algorithms/flatten_product.hh"
#include "properties/Distributable.hh"

using namespace cadabra;

distribute::distribute(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool distribute::can_apply(iterator st)
	{
	//	std::cout << *st->name << std::endl;
	const Distributable *db=kernel.properties.get<Distributable>(st);
	if(!db) {
		return false;
		}

	sibling_iterator facs=tr.begin(st);
	while(facs!=tr.end(st)) {
		if(*(*facs).name=="\\sum" || *(*facs).name=="\\oplus")
			return true;
		//		if(*st->name=="\\indexbracket" || *st->name=="\\diff") break; // only first argument is object
		++facs;
		}
	return false;
	}

Algorithm::result_t distribute::apply(iterator& prod)
	{
	Ex rep;
	rep.set_head(str_node("\\sum", prod->fl.bracket, prod->fl.parent_rel));
	sibling_iterator top=rep.begin();
	// add
	iterator ploc=rep.append_child(top, str_node(prod->name, prod->fl.bracket, prod->fl.parent_rel));
	// The multiplier should sit on each term, not on the sum.
	ploc->multiplier=prod->multiplier;

	// Examine each child node in turn. If it is a sum, distribute it
	// over all previously constructed nodes. Otherwise, add the child
	// node as a child to the previously constructed nodes.

	// "facs" iterates over all child nodes of the distributable (top-level) node
	sibling_iterator facs=tr.begin(prod);
	while(facs!=tr.end(prod)) {
		std::string sumname=*(*facs).name;
		if(sumname=="\\sum" || sumname=="\\oplus") {
			sibling_iterator se=rep.begin(top);
			rep.begin()->name=facs->name;
			// "se" iterates over all nodes in the replacement \sum
			while(se!=rep.end(top)) {
				if(interrupted)
					throw InterruptionException();

				Ex termrep;
				termrep.set_head(str_node());
				sibling_iterator sumch=tr.begin(facs);
				while(sumch!=tr.end(facs)) {
					if(interrupted)
						throw InterruptionException();

					// add product "se" to termrep.
					sibling_iterator dup=termrep.append_child(termrep.begin(), str_node()); // dummy
					dup=termrep.replace(dup, se);
					// add term from sum as factor to product above.
					sibling_iterator newfact=termrep.append_child(dup, sumch);
					// put the multiplier up front
					multiply(dup->multiplier,*newfact->multiplier);
					multiply(dup->multiplier,*facs->multiplier);
					one(newfact->multiplier);
					// make this child inherit the bracket from the sum node
					newfact->fl.bracket=facs->fl.bracket;
					//					newfact->fl.bracket=str_node::b_none;
					++sumch;
					}
				sibling_iterator nxt=se;
				++nxt;
				sibling_iterator sep1=se;
				++sep1;
				se=rep.move_ontop(se, (sibling_iterator)(termrep.begin()));
				rep.flatten(se);
				rep.erase(se);
				//				rep.replace(se, sep1, termrep.begin(termrep.begin()), termrep.end(termrep.begin()));
				se=nxt;
				}
			}
		else {
			sibling_iterator se=rep.begin(top);
			while(se!=rep.end(top)) {
				if(interrupted)
					throw InterruptionException();
				rep.append_child(se, facs);
				++se;
				}
			}
		++facs;
		}
	if(rep.number_of_children(top)==1) { // nothing happened, no sum was present
		//		prod->fl.mark=0; // handled
		// std::cerr << "only one for " << Ex(prod) << std::endl;
		return result_t::l_no_action;
		}

	// FIXME: why does this faster move lead to a crash in linear.cdb?
	iterator ret=tr.move_ontop(prod, (iterator)top);

	// std::cerr << "cleaning up at " << Ex(ret) << std::endl;
	auto sib=tr.begin(ret);
	while(sib!=tr.end(ret)) {
		iterator ii=sib;
		++sib;
		cleanup_dispatch(kernel, tr, ii);
		}
	cleanup_dispatch(kernel, tr, ret);
	// std::cerr << "cleaned up at " << Ex(ret) << std::endl;


	// FIXME: if we had a flattened sum, does the apply_recursive now
	// go and examine every sum that we have created? Should we better
	// return an iterator to the last element in the sum?
	prod=ret;
	return result_t::l_applied;
	}
