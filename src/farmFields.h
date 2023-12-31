#pragma once
#include "deserializationMemo.h"
#include "input.h"
#include "plant.h"
#include "faction.h"
#include "cuboid.h"

#include <list>
#include <unordered_set>

class Block;
class Area;
struct FarmField;


class FarmFieldCreateInputAction final : public InputAction
{
	Cuboid m_cuboid;
	const Faction& m_faction;
	const PlantSpecies& m_species;
	FarmFieldCreateInputAction(InputQueue& inputQueue, Cuboid& cuboid, const Faction& faction, const PlantSpecies& species) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction), m_species(species) { }
	void execute();
};
class FarmFieldRemoveInputAction final : public InputAction
{
	Cuboid m_cuboid;
	const Faction& m_faction;
	FarmFieldRemoveInputAction(InputQueue& inputQueue, Cuboid& cuboid, const Faction& faction ) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction) { }
	void execute();
};
class FarmFieldExpandInputAction final : public InputAction
{
	Cuboid m_cuboid;
	const Faction& m_faction;
	FarmField& m_farmField;
	FarmFieldExpandInputAction(InputQueue& inputQueue, const Faction& faction, Cuboid& cuboid, FarmField& farmField) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction), m_farmField(farmField) { }
	void execute();
};
class FarmFieldUpdateInputAction final : public InputAction
{
	FarmField& m_farmField;
	const PlantSpecies& m_species;
	const Faction& m_faction;
	FarmFieldUpdateInputAction(InputQueue& inputQueue, const Faction& faction, FarmField& farmField, const PlantSpecies& species) : InputAction(inputQueue), m_farmField(farmField), m_species(species), m_faction(faction) { }
	void execute();
};

struct FarmField
{
	std::unordered_set<Block*> blocks;
	const PlantSpecies* plantSpecies;
	bool timeToSow;
	FarmField(std::unordered_set<Block*> b) : blocks(b), plantSpecies(nullptr), timeToSow(false) { }
	FarmField(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction);
	Json toJson() const;
};
class IsPartOfFarmField
{
	Block& m_block;
	std::unordered_map<const Faction*, FarmField*> m_farmFields;
public:
	IsPartOfFarmField(Block& b) : m_block(b) { }
	void insert(const Faction& faction, FarmField& farmField);
	void remove(const Faction& faction);
	void designateForHarvestIfPartOfFarmField(Plant& plant);
	void designateForGiveFluidIfPartOfFarmField(Plant& plant);
	void maybeDesignateForSowingIfPartOfFarmField();
	void removeAllHarvestDesignations();
	void removeAllGiveFluidDesignations();
	void removeAllSowSeedsDesignations();
	bool isSowingSeasonFor(const PlantSpecies& species) const;
	// For testing.
	[[maybe_unused]] bool contains(const Faction& faction) const { return m_farmFields.contains(&faction); }
	[[maybe_unused]] FarmField* get(const Faction& faction) { if(!m_farmFields.contains(&faction)) return nullptr; return m_farmFields.at(&faction); }
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForFaction
{
	Area& m_area;
	const Faction& m_faction;
	std::list<FarmField> m_farmFields;
	std::vector<Plant*> m_plantsNeedingFluid;
	std::unordered_set<Plant*> m_plantsToHarvest;
	std::unordered_set<Block*> m_blocksNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted;
public:
	HasFarmFieldsForFaction(Area& a, const Faction& f) : m_area(a), m_faction(f) { }
	HasFarmFieldsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const Faction& f);
	Json toJson() const;
	void addGivePlantFluidDesignation(Plant& plant);
	void removeGivePlantFluidDesignation(Plant& plant);
	void addSowSeedsDesignation(Block& block);
	void removeSowSeedsDesignation(Block& block);
	void addHarvestDesignation(Plant& plant);
	void removeHarvestDesignation(Plant& plant);
	void setDayOfYear(uint32_t dayOfYear);
	FarmField& create(std::unordered_set<Block*>& blocks);
	FarmField& create(Cuboid cuboid);
	void extend(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void setSpecies(FarmField& farmField, const PlantSpecies& plantSpecies);
	void clearSpecies(FarmField& farmField);
	void designateBlocks(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void shrink(FarmField& farmField, std::unordered_set<Block*>& blocks);
	void remove(FarmField& farmField);
	void undesignateBlocks(std::unordered_set<Block*>& blocks);
	Plant* getHighestPriorityPlantForGiveFluid();
	bool hasHarvestDesignations() const;
	bool hasGivePlantsFluidDesignations() const;
	bool hasSowSeedsDesignations() const;
	const PlantSpecies& getPlantSpeciesFor(const Block& block) const;
};
// To be used by Area.
class HasFarmFields
{
	Area& m_area;
	std::unordered_map<const Faction*, HasFarmFieldsForFaction> m_data;
public:
	HasFarmFields(Area& a) : m_area(a) { } 
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	HasFarmFieldsForFaction& at(const Faction& faction);
	void registerFaction(const Faction& faction);
	void unregisterFaction(const Faction& faction);
	Plant* getHighestPriorityPlantForGiveFluid(const Faction& faction);
	void removeAllSowSeedsDesignations(Block& block);
	void setDayOfYear(uint32_t dayOfYear);
	bool hasGivePlantsFluidDesignations(const Faction& faction) const;
	bool hasHarvestDesignations(const Faction& faction) const;
	bool hasSowSeedsDesignations(const Faction& faction) const;
	const PlantSpecies& getPlantSpeciesFor(const Faction& faction, const Block& location) const;
};
