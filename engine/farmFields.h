#pragma once
#include "designations.h"
#include "geometry/cuboid.h"
#include "geometry/cuboidSet.h"
#include "config/config.h"
#include "dataStructures/smallMap.h"
#include "dataStructures/rtreeBoolean.h"

struct FarmField;
struct DeserializationMemo;
struct Faction;

struct FarmField
{
	CuboidSet m_cuboids;
	PlantSpeciesId m_plantSpecies;
	bool m_timeToSow = false;
	FarmField() = default;
	FarmField(FarmField&& other) noexcept = default;
	FarmField(CuboidSet&& cuboids) : m_cuboids(std::move(cuboids)) { }
	FarmField(const Json& data) { nlohmann::from_json(data, *this); }
	FarmField& operator=(FarmField&& other) noexcept = default;
	[[nodiscard]] bool operator==(const FarmField& other) const { return this == &other; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FarmField, m_cuboids, m_plantSpecies, m_timeToSow);
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForFaction
{
	FactionId m_faction;
	SmallSetStable<FarmField> m_farmFields;
	SmallSet<std::pair<Step, PlantIndex>> m_plantsNeedingFluid;
	bool m_plantsNeedingFluidIsSorted = false;
public:
	HasFarmFieldsForFaction() = default;
	HasFarmFieldsForFaction(const FactionId& f) : m_faction(f) { }
	HasFarmFieldsForFaction(HasFarmFieldsForFaction&& other) noexcept = default;
	HasFarmFieldsForFaction(const HasFarmFieldsForFaction&) { assert(false); std::unreachable(); }
	HasFarmFieldsForFaction& operator=(HasFarmFieldsForFaction&& other) noexcept = default;
	HasFarmFieldsForFaction& operator=(const HasFarmFieldsForFaction&) { assert(false); std::unreachable(); }
	template<typename ShapeT>
	void addGivePlantFluidDesignation(Area& area, const ShapeT& shape);
	void removeGivePlantFluidDesignation(auto& area, const auto& shape) { area.getSpace().designation_unset(shape, m_faction, SpaceDesignation::GivePlantFluid); }
	void maybeRemoveGivePlantFluidDesignation(auto& area, const auto& shape) { area.getSpace().designation_maybeUnset(shape, m_faction, SpaceDesignation::GivePlantFluid); }
	void addSowSeedsDesignation(auto& area, const auto& shape) { area.getSpace().designation_set(shape, m_faction, SpaceDesignation::SowSeeds); }
	void removeSowSeedsDesignation(auto& area, const auto& shape) { area.getSpace().designation_unset(shape, m_faction, SpaceDesignation::SowSeeds); }
	void maybeRemoveSowSeedsDesignation(auto& area, const auto& shape) { area.getSpace().designation_maybeUnset(shape, m_faction, SpaceDesignation::SowSeeds); }
	void addHarvestDesignation(Area& area, const PlantIndex& plant);
	void removeHarvestDesignation(Area& area, const PlantIndex& plant);
	void maybeRemoveHarvestDesignation(auto& area, const auto& shape) { area.getSpace().designation_maybeUnset(shape, m_faction, SpaceDesignation::Harvest); }
	void setDayOfYear(Area& area, int16_t dayOfYear);
	[[nodiscard]] FarmField& create(Area& area, CuboidSet&& cuboids);
	[[nodiscard]] FarmField& create(Area& area, const CuboidSet& cuboids);
	[[nodiscard]] FarmField& create(Area& area, const Cuboid& cuboid);
	void extend(Area& area, FarmField& farmField, CuboidSet& cuboids);
	void setSpecies(Area& area, FarmField& farmField, const PlantSpeciesId& plantSpecies);
	void clearSpecies(Area& area, FarmField& farmField);
	void designatePoints(Area& area, FarmField& farmField, CuboidSet& cuboids);
	void shrink(Area& area, FarmField& farmField, CuboidSet& cuboids);
	void remove(Area& area, FarmField& farmField);
	void undesignatePoints(Area& area, CuboidSet& cuboids);
	void setPointData(Area& area);
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid();
	[[nodiscard]] bool hasGivePlantsFluidDesignations() const;
	[[nodiscard]] bool hasHarvestDesignations(Area& area) const;
	[[nodiscard]] bool hasSowSeedsDesignations(Area& area) const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const Point3D& point) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasFarmFieldsForFaction, m_faction, m_farmFields, m_plantsNeedingFluid, m_plantsNeedingFluidIsSorted);
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
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid(const FactionId& faction);
	void removeAllSowSeedsDesignations(const Point3D& point);
	void setDayOfYear(int16_t dayOfYear);
	[[nodiscard]] bool hasGivePlantsFluidDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasHarvestDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasSowSeedsDesignations(const FactionId& faction) const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const FactionId& faction, const Point3D& location) const;
	// For testing.
	[[maybe_unused, nodiscard]] bool contains(const FactionId& faction) { return m_data.contains(faction); }
};
