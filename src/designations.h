/*
 * Block designations indicate what action a block is suposed to be involved in.
 */
#pragma once

#include <unordered_set>

class Faction;

enum class BlockDesignation { Dig, Construct, SowSeeds, GivePlantFluid, Harvest, StockPileHaulFrom, StockPileHaulTo, Sleep, Rescue };
class HasDesignations
{
	std::unordered_map<const Faction*, std::unordered_set<BlockDesignation>> m_designations;
public:
	bool contains(const Faction& f, const BlockDesignation& bd){ return m_designations[&f].contains(bd); }
	void insert(const Faction& f, const BlockDesignation& bd)
	{
		assert(!m_designations[&f].contains(bd));
	       	m_designations[&f].insert(bd);
       	}
	void remove(const Faction& f, const BlockDesignation& bd)
	{
		assert(m_designations[&f].contains(bd));
	      	m_designations[&f].erase(bd);
	}
	void removeIfExists(const Faction& f, const BlockDesignation& bd) { m_designations[&f].erase(bd); }
};
