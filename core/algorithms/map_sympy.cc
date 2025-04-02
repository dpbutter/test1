
#include "algorithms/map_sympy.hh"
#include "properties/Coordinate.hh"
#include "properties/Symbol.hh"
#include "SympyCdb.hh"

using namespace cadabra;

//#define DEBUG 1

map_sympy::map_sympy(const Kernel& k, Ex& tr, const std::string& head, std::vector<std::string> args=std::vector<std::string>() )
	: Algorithm(k, tr), head_(head), args_(args)
	{
	}

bool map_sympy::can_apply(iterator st)
	{
	// For \components nodes we need to map at the level of the individual
	// component values, not the top \components node.
	if(*st->name=="\\components") return false;
	if(*st->name=="\\equals") return false;
	if(*st->name=="\\comma") return false;
	if(*st->name=="\\arrow") return false;

	left.clear();
	index_factors.clear();
	index_map_t ind_free, ind_dummy;
	classify_indices(st, ind_free, ind_dummy);

	bool still_ok=true;

	// Determine if any of the free indices are harmless (Coordinates or Symbols).
	for(auto& ind: ind_free) {
		const Coordinate *cdn=kernel.properties.get<Coordinate>(ind.second, true);
		const Symbol     *smb=kernel.properties.get<Symbol>(ind.second, true);
		if(cdn==0 && smb==0) {
			still_ok=false;
			break;
			}
		}

	if(still_ok && ind_dummy.size()==0) return true;

	// In a product, it is still possible that there is a sub-product which
	// contains no indices.
	if(*st->name=="\\prod") {
		// Find the factors in the product which have a proper index on them. Do this by
		// starting at the index, and if it is not coordinate or symbol, then go up until we
		// reach the first child level of the product.
		for(auto& ind: ind_free) {
			const Coordinate *cdn=kernel.properties.get<Coordinate>(ind.second, true);
			const Symbol     *smb=kernel.properties.get<Symbol>(ind.second, true);
			if(cdn==0 && smb==0) {
				auto fac=tr.parent(ind.second);
				while(tr.parent(fac)!=iterator(st))
					fac=tr.parent(fac);
				index_factors.insert(fac);
				}
			}
		for(auto& ind: ind_dummy) {
			const Coordinate *cdn=kernel.properties.get<Coordinate>(ind.second, true);
			const Symbol     *smb=kernel.properties.get<Symbol>(ind.second, true);
			if(cdn==0 && smb==0) {
				auto fac=tr.parent(ind.second);
				while(tr.parent(fac)!=iterator(st))
					fac=tr.parent(fac);
				index_factors.insert(fac);
				}
			}
		sibling_iterator sib=tr.begin(st);
		while(sib!=tr.end(st)) {
			if(index_factors.find(iterator(sib))==index_factors.end())
				left.push_back(sib);
			++sib;
			}
		return left.size()>0;
		}

	return false;
	}

Algorithm::result_t map_sympy::apply(iterator& it)
	{
#ifdef DEBUG
	std::cerr << "map_sympy on " << Ex(it) << std::endl;
#endif

	std::vector<std::string> wrap;
	wrap.push_back(head_);

	if(left.size()>0) {
		Ex prod("\\prod");
		for(auto& fac: left)
			prod.append_child(prod.begin(), fac);
		auto top=prod.begin();
		// std::cerr << "Feeding to sympy " << prod << std::endl;
		sympy::apply(kernel, prod, top, wrap, args_, "");
		// Now remove the non-index carrying factors and replace with
		// the factors of 'prod' just simplified.
		tr.insert_subtree(*left.begin(), top);
		// std::cerr << "Before erasing " << Ex(it) << std::endl;
		for(auto& kl: left)
			tr.erase(kl);
		// std::cerr << "After erasing " << Ex(it) << std::endl;

		return result_t::l_applied;
		}
	else {
		sympy::apply(kernel, tr, it, wrap, args_, "");
		it.skip_children();
		return result_t::l_applied;
		}
	}
