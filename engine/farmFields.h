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
struct FarmField
{
	SmallSet<BlockIndex> blocks;
	PlantSpeciesId plantSpecies;
	bool timeToSow = false;
	FarmField() = default;
	FarmField(FarmField&& other) noexcept = default;
	FarmField(SmallSet<BlockIndex>&& b) : blocks(std::move(b)) { }
	FarmField(const Json& data) { nlohmann::from_json(data, *this); }
	FarmField& operator=(FarmField&& other) noexcept = default;
	[[nodiscard]] bool operator==(const FarmField& other) const { return this == &other; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FarmField, blocks, plantSpecies, timeToSow);
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForFaction
{
	FactionId m_faction;
	SmallSetStable<FarmField> m_farmFields;
	SmallSet<BlockIndex> m_blocksWithPlantsNeedingFluid;
	SmallSet<BlockIndex> m_blocksWithPlantsToHarvest;
	SmallSet<BlockIndex> m_blocksNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted = false;
public:
	HasFarmFieldsForFaction() = default;
	HasFarmFieldsForFaction(const FactionId& f) : m_faction(f) { }
	HasFarmFieldsForFaction(HasFarmFieldsForFaction&& other) noexcept = default;
	HasFarmFieldsForFaction& operator=(HasFarmFieldsForFaction&& other) noexcept = default;
	void addGivePlantFluidDesignation(Area& area, const BlockIndex& location);
	void removeGivePlantFluidDesignation(Area& area, const BlockIndex& location);
	void addSowSeedsDesignation(Area& area, const BlockIndex& block);
	void removeSowSeedsDesignation(Area& area, const BlockIndex& block);
	void addHarvestDesignation(Area& area, const PlantIndex& plant);
	void removeHarvestDesignation(Area& area, const PlantIndex& plant);
	void setDayOfYear(Area& area, uint32_t dayOfYear);
	[[nodiscard]] FarmField& create(Area& area, SmallSet<BlockIndex>&& blocks);
	[[nodiscard]] FarmField& create(Area& area, const SmallSet<BlockIndex>& blocks);
	[[nodiscard]] FarmField& create(Area& area, const Cuboid& cuboid);
	[[nodiscard]] FarmField& create(Area& area, const CuboidSet& cuboid);
	void extend(Area& area, FarmField& farmField, SmallSet<BlockIndex>& blocks);
	void setSpecies(Area& area, FarmField& farmField, const PlantSpeciesId& plantSpecies);
	void clearSpecies(Area& area, FarmField& farmField);
	void designateBlocks(Area& area, FarmField& farmField, SmallSet<BlockIndex>& blocks);
	void shrink(Area& area, FarmField& farmField, SmallSet<BlockIndex>& blocks);
	void remove(Area& area, FarmField& farmField);
	void undesignateBlocks(Area& area, SmallSet<BlockIndex>& blocks);
	void setBlockData(Area& area);
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid(Area& area);
	[[nodiscard]] bool hasHarvestDesignations() const;
	[[nodiscard]] bool hasGivePlantsFluidDesignations() const;
	[[nodiscard]] bool hasSowSeedsDesignations() const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const BlockIndex& block) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasFarmFieldsForFaction, m_faction, m_farmFields, m_blocksWithPlantsNeedingFluid, m_blocksWithPlantsToHarvest, m_blocksNeedingSeedsSewn, m_plantsNeedingFluidIsSorted);
};
class AreaHasFarmFields
{
	SmallMap<FactionId, HasFarmFieldsForFaction> m_data;
	Area& m_area;
public:
	AreaHasFarmFields(Area& a) : m_area(a) { }
	void load(const Json& data);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] HasFarmFieldsForFaction& getForFaction(const FactionId& faction);
	void registerFaction(const FactionId& faction);
	void unregisterFaction(const FactionId& faction);
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid(Area& area, const FactionId& faction);
	void removeAllSowSeedsDesignations(const BlockIndex& block);
	void setDayOfYear(uint32_t dayOfYear);
	[[nodiscard]] bool hasGivePlantsFluidDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasHarvestDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasSowSeedsDesignations(const FactionId& faction) const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const FactionId& faction, const BlockIndex& location) const;
	// For testing.
	[[maybe_unused, nodiscard]] bool contains(const FactionId& faction) { return m_data.contains(faction); }
};
