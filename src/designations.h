/*
 * Block designations indicate what action a block is suposed to be involved in.
 */
#pragma once

#include <unordered_set>
#include <unordered_map>
#include <cassert>

class Faction;

enum class BlockDesignation { Dig, Construct, SowSeeds, GivePlantFluid, Harvest, StockPileHaulFrom, StockPileHaulTo, Sleep, Rescue, FluidSource };
class HasDesignations
{
	std::unordered_map<const Faction*, std::unordered_set<BlockDesignation>> m_designations;
public:
	bool contains(const Faction& f, const BlockDesignation& bd) const 
	{ 
		if(!m_designations.contains(&f))
			return false;
		return m_designations.at(&f).contains(bd); 
	}
	void insert(const Faction& f, const BlockDesignation& bd)
	{
		assert(!m_designations[&f].contains(bd));
	       	m_designations[&f].insert(bd);
       	}
	void remove(const Faction& f, const BlockDesignation& bd)
	{
		assert(m_designations[&f].contains(bd));
		if(m_designations[&f].size() == 1)
			m_designations.erase(&f);
		else
	      		m_designations[&f].erase(bd);
	}
	void removeIfExists(const Faction& f, const BlockDesignation& bd) { m_designations[&f].erase(bd); }
};
