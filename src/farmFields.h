#pragma once
#include "plant.h"
#include "faction.h"

#include <list>
#include <unordered_set>

class Block;

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
	std::unordered_map<Faction*, FarmField*> m_farmFields;
public:
	IsPartOfFarmField(Block& b) : m_block(b) { }
	void insert(Faction& faction, FarmField& farmField);
	void remove(Faction& faction);
	void designateForHarvestIfPartOfFarmField(Plant& plant);
	void designateForGiveFluidIfPartOfFarmField(Plant& plant);
	void removeAllHarvestDesignations();
	void removeAllGiveFluidDesignations();
	void removeAllSowSeedsDesignations();
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForFaction
{
	Faction& m_faction;
	std::list<FarmField> m_farmFields;
	std::vector<Plant*> m_plantsNeedingFluid;
	std::unordered_set<Plant*> m_plantsToHarvest;
	std::unordered_set<Block*> m_blocksNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted;
public:
	HasFarmFieldsForFaction(Faction& p) : m_faction(p) { }
	bool hasGivePlantsFluidDesignations() const;
	bool hasSowSeedsDesignations() const;
	const PlantSpecies& getPlantTypeFor(Block& block) const;
	void addGivePlantFluidDesignation(Plant& plant);
	void removeGivePlantFluidDesignation(Plant& plant);
	void addSowSeedsDesignation(Block& block);
	void removeSowSeedsDesignation(Block& block);
	void addHarvestDesignation(Plant& plant);
	void removeHarvestDesignation(Plant& plant);
	void setDayOfYear(uint32_t dayOfYear);
	void create(std::unordered_set<Block*>& blocks);
	void extend(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void setSpecies(FarmField& farmField, const PlantSpecies& plantSpecies);
	void designateBlocks(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void clearSpecies(FarmField& farmField);
	void shrink(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void remove(FarmField& farmField);
	void undesignateBlocks(std::unordered_set<Block*>& blocks);
	Plant* getHighestPriorityPlantForGiveFluid();
	bool hasHarvestDesignations() const;
};
// To be used by Area.
class HasFarmFields
{
	std::unordered_map<Faction*, HasFarmFieldsForFaction> m_data;
public:
	HasFarmFieldsForFaction& at(Faction& faction);
	void registerFaction(Faction& faction);
	void unregisterFaction(Faction& faction);
	Plant* getHighestPriorityPlantForGiveFluid(Faction& faction);
	bool hasGivePlantsFluidDesignations(Faction& faction) const;
	bool hasHarvestDesignations(Faction& faction) const;
};
