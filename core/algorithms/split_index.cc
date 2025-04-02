
#include "algorithms/split_index.hh"

// #define DEBUG 1

using namespace cadabra;

split_index::split_index(const Kernel& k, Ex& tr, Ex& triple)
	: Algorithm(k, tr), part1_class(0), part2_class(0),
	  part1_coord(0), part2_coord(0), part1_is_number(false), part2_is_number(false)
	{
	iterator top=triple.begin();
	if(*(top->name)!="\\comma") {
		std::cout << "not comma" << std::endl;
		throw ArgumentException("split_index: Need a list of three index names.");
		}
	else if(triple.number_of_children(top)!=3) {
		std::cout << "not 3" << std::endl;
		throw ArgumentException("split_index: Need a list of three (no more, no less) index names.");
		}

	sibling_iterator iname=triple.begin(top);
	full_class=kernel.properties.get<Indices>(iname, true);
	++iname;
	if(iname->is_integer()) {
		part1_is_number=true;
		num1=to_long(*(iname->multiplier));
		}
	else {
		part1_class=kernel.properties.get<Indices>(iname, true);
		part1_coord=kernel.properties.get<Coordinate>(iname, true);
		if(part1_coord) part1_coord_node=iname;
		}
	++iname;
	if(iname->is_integer()) {
		part2_is_number=true;
		num2=to_long(*(iname->multiplier));
		}
	else {
		part2_class=kernel.properties.get<Indices>(iname, true);
		part2_coord=kernel.properties.get<Coordinate>(iname, true);
		if(part2_coord) part2_coord_node=iname;
		}
	if(full_class && (part1_is_number || part1_class || part1_coord) && (part2_is_number || part2_class || part2_coord) )
		return;

	throw ArgumentException("split_index: The index types of (some of) these indices are not known.");
	}

bool split_index::can_apply(iterator it)
	{
	if((tr.is_head(it) && (*it->name!="\\equals" && *it->name!="\\sum"))) return true;
	if(!tr.is_head(it)) {
		if((*tr.parent(it)->name=="\\equals" && *it->name!="\\sum") ||
		      (*tr.parent(it)->name=="\\sum") )
			return true;
		}

	return false;

	// This does not work well:
	//	  return is_termlike(it);
	// The problem is that split_index has a very restricted applicability,
	// which is less than on what the core thinks is 'term-like'. Hence
	// the hard-coded logic above.
	}

Algorithm::result_t split_index::apply(iterator& it)
	{
	result_t ret=result_t::l_no_action;

	Ex rep;
	rep.set_head(str_node("\\sum"));
	Ex workcopy(it); // so we can make changes without spoiling the big tree
	//	assert(*it->multiplier==1); // see if this made a difference

#ifdef DEBUG
	std::cerr << "split index acting at " << it << std::endl;
#endif

	// we only replace summed indices, so first find them.
	index_map_t ind_free, ind_dummy;
	classify_indices(workcopy.begin(), ind_free, ind_dummy);
	//	txtout << "indices classified" << std::endl;

	index_map_t::iterator prs=ind_dummy.begin();
	while(prs!=ind_dummy.end()) {
		const Indices *tcl=kernel.properties.get<Indices>((*prs).second, true);
		if(tcl) {
			if((*tcl).set_name==(*full_class).set_name) {
				Ex dum1,dum2;
				if(!part1_is_number && !part1_coord)
					dum1=get_dummy(part1_class, it);
				index_map_t::iterator current=prs;
				while(current!=ind_dummy.end() && tree_exact_equal(&kernel.properties, (*prs).first,(*current).first,true)) {
					if(part1_is_number) {
						node_integer(current->second, num1);
						//						(*prs).second->name=name_set.insert(to_string(num1)).first;
						}
					else if(part1_coord) {
						(*current).second=tr.replace_index((*current).second, part1_coord_node, true);
						}
					else {
						//						txtout << "going to replace" << std::endl;
						(*current).second=tr.replace_index((*current).second, dum1.begin(), true);
						//						txtout << "replaced" << std::endl;
						}
					// Important: restoring (*prs).second in the line above.
					++current;
					}
				rep.append_child(rep.begin(), workcopy.begin());
				current=prs;
				if(!part2_is_number && !part2_coord)
					dum2=get_dummy(part2_class, it);
				while(current!=ind_dummy.end() && tree_exact_equal(&kernel.properties, (*prs).first,(*current).first,true)) {
					if(part2_is_number) {
						node_integer(current->second, num2);
						//						(*prs).second->name=name_set.insert(to_string(num2)).first;
						}
					else if(part2_coord) {
						(*current).second=tr.replace_index((*current).second, part2_coord_node, true);
						}
					else tr.replace_index((*current).second,dum2.begin(), true);
					++current;
					}
				rep.append_child(rep.begin(), workcopy.begin());
				// Do not copy the multiplier; it has already been copied by cloning the original into workcopy.
				//	rep.begin()->multiplier=it->multiplier;
				//				txtout << "cleaning up" << std::endl;
				//				rep.print_recursive_treeform(txtout, rep.begin());
				it=tr.replace(it, rep.begin());

				// FIXME: need to cleanup nests

				//				cleanup_nests(tr, it);

				ret=result_t::l_applied;
				break;
				}
			}
		// skip other occurrances of this index
		index_map_t::iterator current=prs;
		while(prs!=ind_dummy.end() && tree_exact_equal(&kernel.properties, (*prs).first,(*current).first,false))
			++prs;
		}

	return ret;
	}
