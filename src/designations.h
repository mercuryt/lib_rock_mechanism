/*
 * Block designations indicate what action a block is suposed to be involved in.
 */
#pragma once
#include <unordered_set>

enum class BlockDesignation { Dig, Construct, SowSeeds, GivePlantFluid, Harvest, StockPileHaulFrom, StockPileHaulTo, Sleep };
class HasDesignations
{
	std::unordered_map<const Player*, std::unordered_set<BlockDesignation>> m_designations;
public:
	bool contains(const Player* p, BlockDesignation& bd){ return m_designations[p].contains(bd); }
	void insert(const Player* p, BlockDesignation& bd){ m_designations[p].insert(bd); }
	void remove(const Player* p, BlockDesignation& bd){ m_designations[p].remove(bd); }
};
