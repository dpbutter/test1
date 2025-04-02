
#include "algorithms/sym.hh"

using namespace cadabra;

// #define DEBUG 1

sym::sym(const Kernel& k, Ex& tr, const std::vector<unsigned int>& slots, bool s)
	: Algorithm(k, tr), sign(s), slotloc(slots)
	{
#ifdef DEBUG
	std::cerr << "sym::sym: initialised with slot positions." << std::endl;
#endif
	}

sym::sym(const Kernel& k, Ex& tr, Ex& objs, bool s)
	: Algorithm(k, tr), objects(objs), sign(s)
	{
	if(objects.size()==0)
		throw ArgumentException("sym/asym: need a list of objects over which to (anti)symmetrise.");
	}

bool sym::can_apply(iterator it)
	{
	if(*it->name!="\\prod")
		if(!is_single_term(it))
			return false;

	prod_wrap_single_term(it);
	bool located=false;
	if(objects.size()>0) {
#ifdef DEBUG
		std::cerr << "sym::can_apply: finding objects" << std::endl;
#endif
		argloc_2_treeloc.clear();
		located=locate_object_set(objects, tr.begin(it), tr.end(it), argloc_2_treeloc);
		}
	else {
#ifdef DEBUG
		std::cerr << "sym::can_apply: collecting objects" << std::endl;
#endif
		objects.set_head(str_node("\\comma"));
		argloc_2_treeloc.clear();
		argloc_2_treeloc.resize(slotloc.size(), 0);
		for(size_t i=0; i<slotloc.size(); ++i) {
			auto ind=begin_index(it);
			ind+=slotloc[i];
			if(tr.is_valid(ind)==false) {
				throw ArgumentException("Expression does not have at least "+std::to_string(slotloc[i]+1)+" indices.");
				}
#ifdef DEBUG
			std::cerr << "sym::can_apply: adding " << ind << std::endl;
#endif
			objects.append_child(objects.begin(), Ex::iterator(ind));

			// Convert the index-index to iterator-index (the former says how many times
			// an IndexIterator should be incremented, the latter says how many times a
			// generic Iterator should be incremented).
			iterator steps=tr.begin(it);
			while(steps!=iterator(ind)) {
				++steps;
				++argloc_2_treeloc[i];
				}
			}
		located=true;
		}
	prod_unwrap_single_term(it);
#ifdef DEBUG
	std::cerr << "sym::can_apply: ready to go: " << located << std::endl;
#endif
	return located;
	}

Algorithm::result_t sym::apply(iterator& it)
	{
#ifdef DEBUG
	std::cerr << "sym::apply: running" << std::endl;
#endif

	prod_wrap_single_term(it);
	result_t res=doit(it,sign);
	//	if(res==result_t::l_applied)
	//		it=tr.parent(st);

	//IS DOIT not doing that right?

	return res;
	}

Algorithm::result_t sym::doit(iterator& it, bool sign)
	{
	assert(*it->name=="\\prod");

#ifdef DEBUG
	std::cerr << "sym::doit: " << objects << std::endl;
#endif
	
	// Setup combinations class. First construct original and block length.
	sibling_iterator fst=objects.begin(objects.begin());
	sibling_iterator fnd=objects.end(objects.begin());
	raw_ints.clear();
	raw_ints.block_length=0;

	for(unsigned int i=0; i<argloc_2_treeloc.size(); ++i)
		raw_ints.original.push_back(i);
	while(fst!=fnd) {
#ifdef DEBUG
		std::cerr << "sym::doit: object " << *fst->name	<< std::endl;
#endif
		if(*(fst->name)=="\\comma") {
			if(raw_ints.block_length==0) raw_ints.block_length=tr.number_of_children(fst);
			else                         assert(raw_ints.block_length==tr.number_of_children(fst));
			}
		else if(fst->name->size()>0 || (fst->name->size()==0 && tr.number_of_children(fst)==1)) {
			if(raw_ints.block_length==0) raw_ints.block_length=1;
			else                         assert(raw_ints.block_length==1);
			}
		++fst;
		}
	long start_=-1, end_=-1;


	// FIXME: what was this v1 feature supposed to do?
	//
	//	sibling_iterator other_args=args_begin();
	//	++other_args;
	//	while(other_args!=args_end()) {
	//		if(*(other_args->name)=="\\setoption") {
	//			if(*tr.child(other_args,0)->name=="Start")
	//				start_=to_long(*tr.child(other_args,1)->multiplier);
	//			else if(*tr.child(other_args,0)->name=="End")
	//				end_=to_long(*tr.child(other_args,1)->multiplier);
	//			}
	//		++other_args;
	//		}

	raw_ints.set_unit_sublengths();
	// Sort within the blocks, if any
	if(raw_ints.block_length!=1) {
		std::vector<unsigned int>::iterator fr=argloc_2_treeloc.begin();
		std::vector<unsigned int>::iterator to=argloc_2_treeloc.begin();
		to+=raw_ints.block_length;
		for(unsigned int i=0; i<raw_ints.original.size()/raw_ints.block_length; ++i) {
			std::sort(fr, to);
			fr+=raw_ints.block_length;
			to+=raw_ints.block_length;
			}
		}
	//	txtout << raw_ints.original.size() << " original size" << std::endl;
	//	txtout << raw_ints.block_length << " block length" << std::endl;

	// Add output asym ranges.
	// FIXME: v2: this is probably not very useful for the average user.
	//	if(number_of_args()>1) {
	//		sibling_iterator ai=args_begin();
	//		++ai;
	//		while(ai!=args_end()) {
	//			if(*ai->name=="\\comma") {
	//				sibling_iterator cst=tr.begin(ai);
	//				combin::range_t asymrange;
	//				while(cst!=tr.end(ai)) {
	//					asymrange.push_back(to_long(*cst->multiplier));
	//					++cst;
	//					}
	//				raw_ints.input_asym.push_back(asymrange);
	//				}
	//			++ai;
	//			}
	//		}

	raw_ints.permute(start_, end_);

#ifdef DEBUG
	std::cerr << "Computed all permutations: " << raw_ints.size() << std::endl;
#endif

	// Build replacement tree.
	Ex rep;
	sibling_iterator top=rep.set_head(str_node("\\sum"));
	sibling_iterator dummy=rep.append_child(top, str_node("dummy"));

	for(unsigned int i=0; i<raw_ints.size(); ++i) {
		Ex copytree(it);// CORRECT?
		copytree.begin()->fl.bracket=str_node::b_none;
		copytree.begin()->fl.parent_rel=str_node::p_none;

		std::map<iterator, iterator, Ex::iterator_base_less> replacement_map;

		for(unsigned int j=0; j<raw_ints[i].size(); ++j) {
			iterator repl=copytree.begin(), orig=it; // CORRECT?
			++repl;
			++orig;
			for(unsigned int k=0; k<argloc_2_treeloc[raw_ints[i][j]]; ++k)
				++orig;
			for(unsigned int k=0; k<argloc_2_treeloc[raw_ints.original[j]]; ++k)
				++repl;

			// We cannot just replace here, because then walking along the tree
			// in the next step may no longer work (we may be swapping objects
			// with different numbers of indices, as in
			//
			//   A_{a b} B_{c};
			//   @sym!(%){A_{a b}, B_{c}};
			//
			// so we store iterators first.

			if((*orig->name).size()==0)
				replacement_map[repl]=tr.begin(orig);
			else
				replacement_map[repl]=orig;
			}

		// All replacement rules now figured out, let's do them.
		std::map<iterator, iterator>::iterator rit=replacement_map.begin();
		while(rit!=replacement_map.end()) {
			str_node::bracket_t cbr=rit->first->fl.bracket;
			iterator repl=copytree.replace(rit->first, rit->second);
			// FIXME: think about whether this is what we want: the bracket
			// type 'stays', while the parent rel is moved together with the
			// index. A(x)*Z[y] -> A(y)*Z[x] ,
			//        A^m_n     -> A_n^m .
			repl->fl.bracket=cbr;
			++rit;
			}

		// Some final multiplier stuff and cleanup

		multiply(copytree.begin()->multiplier, 1/multiplier_t(raw_ints.total_permutations()));
		//		multiply(copytree.begin()->multiplier, *st->multiplier);
		if(sign)
			multiply(copytree.begin()->multiplier, raw_ints.ordersign(i));

		iterator tmp=copytree.begin();
		prod_unwrap_single_term(tmp);
		rep.insert_subtree(dummy, copytree.begin());
#ifdef DEBUG
		if(i%1000==0)
			std::cerr << i << std::endl;
#endif
		}
	rep.erase(dummy);

	// show replacement tree
	//	txtout << "replacement : " << std::endl;
	//	eo.print_infix(rep.begin());
	//	txtout << std::endl;

	it=tr.replace(it, rep.begin());
	//	if(*(tr.parent(reploc)->name)=="\\sum") {
	//		tr.flatten(reploc);
	//		reploc=tr.erase(reploc);
	//		}
	return result_t::l_applied;
	}
