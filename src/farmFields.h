#pragma once
#include "plant.h"
#include "player.h"
#include "block.h"

#include <list>
#include <unordered_set>

struct FarmField
{
	std::unordered_set<Block*> blocks;
	const PlantSpecies* plantSpecies;
	bool timeToSow;
	FarmField(std::unordered_set<Block*> b) : blocks(b), plantSpecies(nullptr) { }
};
class IsPartOfFarmField
{
	Block& m_block;
	std::unordered_map<Player*, FarmField*> m_farmFields;
	IsPartOfFarmField(Block& b) : m_block(b) { }
public:
	void insert(Player& player, FarmField& farmField);
	void remove(Player& player);
	void designateForHarvestIfPartOfFarmField(Plant& plant)
	{
		for(auto& [player, farmField] : m_farmFields)
			if(farmField.plantSpecies == plant.plantSpecies)
				block.m_area->m_hasFarmFields.at(player).addHarvestDesignation(plant);
	}
	void designateForGiveFluidIfPartOfFarmField(Plant& plant)
	{
		for(auto& [player, farmField] : m_farmFields)
			if(farmField.plantSpecies == plant.plantSpecies)
				block.m_area->m_hasFarmFields.at(player).addGivePlantFluidDesignation(plant);
	}
	void removeAllHarvestDesignations()
	{
		Plant& plant = m_block.m_hasPlant.get();	
		for(auto& [player, farmField] : m_farmFields)
			if(plant.plantSpecies == farmField.plantSpecies)
				m_block.m_area->m_hasFarmFields.at(player).removeHarvestDesignation(plant);
	}
	void removeAllGiveFluidDesignations()
	{
		Plant& plant = m_block.m_hasPlant.get();	
		for(auto& [player, farmField] : m_farmFields)
			if(plant.plantSpecies == farmField.plantSpecies)
				m_block.m_area->m_hasFarmFields.at(player).removeGiveFluidDesignation(plant);
	}
	void removeAllSowSeedsDesignations()
	{
		for(auto& [player, farmField] : m_farmFields)
			m_block.m_area->m_hasFarmFields.at(player).removeSowSeedsDesignation(m_block);
	}
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForPlayer
{
	Player& m_player;
	std::list<FarmField> m_farmFields;
	std::unordered_set<Plant*> m_plantsNeedingFluid;
	std::unordered_set<Plant*> m_plantsToHarvest;
	std::unordered_set<Block*> m_blocksNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted;
public:
	bool hasGivePlantsFluidDesignations() const;
	Plant* getHighPriorityPlantForGivingFluidIfAny();
	bool hasSowSeedDesignations() const;
	const PlantSpecies& getPlantTypeFor(Block& block) const;
	void addGivePlantFluidDesignation(Plant& plant);
	void removeGivePlantFluidDesignation(Plant& plant);
	void addSowSeedDesignation(Block& block);
	void removeSowSeedDesignation(Block& block);
	void addHarvestDesignation(Plant& plant);
	void removeHarvestDesignation(Plant& plant);
	void setDayOfYear(uint32_t dayOfYear);
	void create(std::unordered_set<Block*>& blocks);
	void extend(FarmField& farmField, std:unordered_set<Block*>& blocks);
	void setSpecies(FarmField& farmField, const PlantSpecies& plantSpecies);
	void designateBlocks(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void clearSpecies(FarmField& farmField);
	void shrink(FarmField& farmField, std::unordered_set<Block*>& blocks);
};
// To be used by Area.
class HasFarmFields
{
	std::unordered_map<Player*, HasFarmFieldsForPlayer> m_data;
public:
	void at(Player& player);
	void registerPlayer(Player& player);
	void unregisterPlayer(Player& player);
};
