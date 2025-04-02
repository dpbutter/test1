
#include "explicit_indices.hh"

#include "Cleanup.hh"

#include "algorithms/substitute.hh"
#include "properties/ImplicitIndex.hh"
#include "properties/PartialDerivative.hh"
#include "properties/Trace.hh"

using namespace cadabra;

explicit_indices::explicit_indices(const Kernel& k, Ex& tr)
	: Algorithm(k, tr)
	{
	}

bool explicit_indices::can_apply(iterator st)
	{
	// Work on equals nodes, or single terms (not terms in a sum) or
	// sums, provided the latter are not lhs or rhs of an equals node.
	// All this because when we generate free indices, we need to
	// ensure that all terms and all sides of an equals node use the
	// same index names.

	if(*st->name=="\\equals" || *st->name=="\\arrow") {
		sibling_iterator lhs=tr.begin(st);
		if(can_apply(lhs)) {
			++lhs;
			if(can_apply(lhs))
				return true;
			}
		return false;
		}
	auto trace = kernel.properties.get<Trace>(st);
	if(trace)   return true;
	if(is_termlike(st) || *st->name=="\\sum") {
		if(tr.is_head(st)) return         true;
		if(*tr.parent(st)->name=="\\sum") return false;
		if(*tr.parent(st)->name=="\\equals" || *tr.parent(st)->name=="\\arrow") return false;
		auto ptrace = kernel.properties.get<Trace>(tr.parent(st));
		if(ptrace) return false;
		auto pderiv = kernel.properties.get<PartialDerivative>(tr.parent(st));
		if(pderiv) return false;
		return true;
		}

	return false;
	}

Algorithm::result_t explicit_indices::apply(iterator& it)
	{
	result_t res=result_t::l_no_action;

	if(*it->name=="\\equals" || *it->name=="\\arrow") {
		// FIXME: collect existing indices on both sides to avoid
		// adding new free indices which are already in use on the
		// other side.

		// Do the lhs, keep track of free indices added there.
		iterator ch=tr.begin(it);
		auto lhs_res = apply(ch);

		// Then do the rhs, feeding it the free indices just generated.
		++ch;
		auto rhs_res = apply(ch);

		if(lhs_res==result_t::l_applied || rhs_res==result_t::l_applied)
			return result_t::l_applied;
		return result_t::l_no_action;
		}

	// All non-equality and non-rule cases follow.
	auto trace = kernel.properties.get<Trace>(it);
	iterator parit=it;
	if(trace) {
		// Handle the argument, then at the end, close the index loop
		// and remove the trace operator.
		iterator arg=tr.begin(it);
		if(! (*arg->name=="\\sum" || is_termlike(arg)))
			return res;
		it=arg;
		}

	// Ensure that we are always working on a sum, even
	// if there is only one term.
	if(is_termlike(it))
		force_node_wrap(it, "\\sum");

	// Classify all free and dummy indices already present. Any new
	// indices cannot be taken from these.
//	std::cerr << "acting at " << it << std::endl;
	
	ind_free_sum.clear();
	ind_dummy_sum.clear();
	classify_indices(it, ind_free_sum, ind_dummy_sum);

	sibling_iterator term=tr.begin(it);
	while(term!=tr.end(it)) {
		sibling_iterator nxt=term;
		++nxt;

		iterator tmp=term;
		prod_wrap_single_term(tmp);
		term=tmp;

		// For each index set, keep track of the last used index in
		// building the explicit index line.
		added_this_term.clear();
		index_lines.clear();
		first_index.clear();
		last_index.clear();

		sibling_iterator factor=tr.begin(term);
		while(factor!=tr.end(term)) {
			const PartialDerivative *pd = kernel.properties.get<PartialDerivative>(factor);
			if(pd) {
				sibling_iterator args=tr.begin(factor);
				while(args!=tr.end(factor)) {
					if(args->fl.parent_rel==str_node::p_none) {
						res=result_t::l_applied;
						handle_factor(args, trace!=0);
						break;
						}
					++args;
					}
				}
			else {
				res=result_t::l_applied;				
				handle_factor(factor, trace!=0);
				}
			++factor;
			}
		// If this term is a trace, make indices equal
		if(trace) {
			for(auto& li: last_index) {
				tr.replace_index(li.second, first_index[li.first], true);
				}
			}


		tmp=term;
		prod_unwrap_single_term(tmp);

		//		std::cerr << "after unwrap" << tmp << std::endl;

		term=nxt;
		}

	if(trace) {
		it=parit;
		it = tr.flatten_and_erase(it);
		}
	if(*it->name=="\\sum" && tr.number_of_children(it)==1)
		it = tr.flatten_and_erase(it);
	// It looks like we will have trouble if we also flatten the
	// \prod node, becuase then rename_dummies will only act on
	// the first factor we introduced, and fail to rename
	// properly.
//	cleanup_dispatch(kernel, tr, it);

//	std::cerr << "after explicit_indicex: " << tr.begin() << std::endl;
//	std::cerr << "and it is now " << it << std::endl;
	
	return res;
	}

void explicit_indices::handle_factor(sibling_iterator& factor, bool )
	{
	int tmp;
	auto ii = kernel.properties.get_with_pattern<ImplicitIndex>(factor, tmp, "");
	if(ii.first) {
		// Determine indices on this factor. Use a copy because we
		// need this object later.
		Ex orig(factor);
		index_map_t factor_ind_free, factor_ind_dummy;
		classify_indices(orig.begin(), factor_ind_free, factor_ind_dummy);

		// Substitute explcit replacement and rename the indices
		// already present on the implicit factor.
		Ex replacement("\\arrow");
		replacement.append_child(replacement.begin(), ii.second->obj.begin());
		replacement.append_child(replacement.begin(), ii.first->explicit_form.begin());
		substitute subs(kernel, tr, replacement);
		iterator factor_tmp=factor;
		if(subs.can_apply(factor_tmp))
			subs.apply(factor_tmp);
		else
			throw InternalError("Internal inconsistency encountered, aborting.");
		factor=factor_tmp;

		// Determine indices on the replacement.
		index_map_t repl_ind_free, repl_ind_dummy;
		classify_indices(factor, repl_ind_free, repl_ind_dummy);

		// Which indices have been added?
		index_map_t ind_same;
		IndexClassifier ic(kernel);
		ic.determine_intersection(factor_ind_free, repl_ind_free, ind_same, true);

		// Keep track of indices already added in this factor, to avoid making
		// an index trace on a single factor.
		std::map<const Indices *, Ex::iterator> index_lines_factor;

		// Go through indices in order of appearance, determine if they have
		// been added, and rename. Note: indices do not appear in order in the
		// index maps above.
		auto iit=index_iterator::begin(kernel.properties, factor);
		while(iit!=index_iterator::end(kernel.properties, factor)) {
			auto search=repl_ind_free.begin();
			bool found=false;
			while(search!=repl_ind_free.end()) {
				if(search->second == (iterator)iit) {
					found=true;
					break;
					}
				++search;
				}
			++iit; // Update now, we may be replacing this index.
			if(found) {
				// This index was added.
				const Indices *ip = kernel.properties.get<Indices>(search->second);
				if(!ip)
					throw InternalError("Do not have Indices property for all implicit indices.");

				// Determine if we have an 'active' index line for
				// this index type.
				auto line        = index_lines.find(ip);
				auto line_factor = index_lines_factor.find(ip);
				if(line==index_lines.end() || line_factor!=index_lines_factor.end()) {
					// No active line. Get a new free index.
					auto di = ic.get_dummy(ip, &ind_free_sum, &ind_dummy_sum, &added_this_term);
					auto loc = tr.replace_index(search->second, di.begin(), true);
					added_this_term.insert(index_map_t::value_type(di, loc));
					index_lines[ip]=loc;
					index_lines_factor[ip]=loc;
					last_index[ip]=loc;
					auto first = first_index.find(ip);
					if(first==first_index.end()) {
						first_index[ip]=loc;
						}
					}
				else {
					// Use the active line index, then unset the active line.
					tr.replace_index(search->second, line->second, true);
					index_lines.erase(line);
					}
				}
			}
		}
	}
