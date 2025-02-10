#pragma once
//#include "input.h"
#include "plantSpecies.h"
#include "geometry/cuboid.h"
#include "geometry/cuboidSet.h"
#include "config.h"

#include <list>

class Area;
struct FarmField;
struct DeserializationMemo;
struct Faction;

/*
class FarmFieldCreateInputAction final : public InputAction
{
	Cuboid m_cuboid;
	FactionId m_faction;
	PlantSpeciesId m_species;
	FarmFieldCreateInputAction(InputQueue& inputQueue, Cuboid& cuboid, const FactionId& faction, PlantSpeciesId species) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction), m_species(species) { }
	void execute();
};
class FarmFieldRemoveInputAction final : public InputAction
{
	Cuboid m_cuboid;
	FactionId m_faction;
	FarmFieldRemoveInputAction(InputQueue& inputQueue, Cuboid& cuboid, const FactionId& faction ) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction) { }
	void execute();
};
class FarmFieldExpandInputAction final : public InputAction
{
	Cuboid m_cuboid;
	FactionId m_faction;
	FarmField& m_farmField;
	FarmFieldExpandInputAction(InputQueue& inputQueue, const FactionId& faction, Cuboid& cuboid, FarmField& farmField) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction), m_farmField(farmField) { }
	void execute();
};
class FarmFieldUpdateInputAction final : public InputAction
{
	FarmField& m_farmField;
	PlantSpeciesId m_species;
	FactionId m_faction;
	FarmFieldUpdateInputAction(InputQueue& inputQueue, const FactionId& faction, FarmField& farmField, PlantSpeciesId species) : InputAction(inputQueue), m_farmField(farmField), m_species(species), m_faction(faction) { }
	void execute();
};
*/
// TODO: Make POD.
struct FarmField
{
	SmallSet<BlockIndex> blocks;
	Area& area;
	PlantSpeciesId plantSpecies;
	bool timeToSow;
	FarmField(Area& a, SmallSet<BlockIndex>&& b) : blocks(std::move(b)), area(a), timeToSow(false) { }
	FarmField(const Json& data, const FactionId& faction, Area& area);
	[[nodiscard]] Json toJson() const;
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForFaction
{
	Area& m_area;
	FactionId m_faction;
	std::list<FarmField> m_farmFields;
	SmallSet<BlockIndex> m_blocksWithPlantsNeedingFluid;
	SmallSet<BlockIndex> m_blocksWithPlantsToHarvest;
	SmallSet<BlockIndex> m_blocksNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted = false;
public:
	HasFarmFieldsForFaction(Area& a, const FactionId& f) : m_area(a), m_faction(f) { }
	HasFarmFieldsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const FactionId& f);
	[[nodiscard]] Json toJson() const;
	void addGivePlantFluidDesignation(const PlantIndex& plant);
	void removeGivePlantFluidDesignation(const PlantIndex& plant);
	void addSowSeedsDesignation(const BlockIndex& block);
	void removeSowSeedsDesignation(const BlockIndex& block);
	void addHarvestDesignation(const PlantIndex& plant);
	void removeHarvestDesignation(const PlantIndex& plant);
	void setDayOfYear(uint32_t dayOfYear);
	[[nodiscard]] FarmField& create(SmallSet<BlockIndex>&& blocks);
	[[nodiscard]] FarmField& create(const BlockIndices& blocks);
	[[nodiscard]] FarmField& create(const Cuboid& cuboid);
	[[nodiscard]] FarmField& create(const CuboidSet& cuboid);
	void extend(FarmField& farmField, SmallSet<BlockIndex>& blocks);
	void setSpecies(FarmField& farmField, const PlantSpeciesId& plantSpecies);
	void clearSpecies(FarmField& farmField);
	void designateBlocks(FarmField& farmField, SmallSet<BlockIndex>& blocks);
	void shrink(FarmField& farmField, SmallSet<BlockIndex>& blocks);
	void remove(FarmField& farmField);
	void undesignateBlocks(SmallSet<BlockIndex>& blocks);
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid();
	[[nodiscard]] bool hasHarvestDesignations() const;
	[[nodiscard]] bool hasGivePlantsFluidDesignations() const;
	[[nodiscard]] bool hasSowSeedsDesignations() const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const BlockIndex& block) const;
};
// To be used by Area.
class AreaHasFarmFields
{
	std::unordered_map<FactionId, HasFarmFieldsForFaction, FactionId::Hash> m_data;
	Area& m_area;
public:
	AreaHasFarmFields(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] HasFarmFieldsForFaction& getForFaction(const FactionId& faction);
	void registerFaction(const FactionId& faction);
	void unregisterFaction(const FactionId& faction);
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid(const FactionId& faction);
	void removeAllSowSeedsDesignations(const BlockIndex& block);
	void setDayOfYear(uint32_t dayOfYear);
	[[nodiscard]] bool hasGivePlantsFluidDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasHarvestDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasSowSeedsDesignations(const FactionId& faction) const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const FactionId& faction, const BlockIndex& location) const;
	// For testing.
	[[maybe_unused, nodiscard]] bool contains(const FactionId& faction) { return m_data.contains(faction); }
};
