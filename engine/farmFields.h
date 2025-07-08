#pragma once
//#include "input.h"
#include "definitions/plantSpecies.h"
#include "geometry/cuboid.h"
#include "geometry/cuboidSet.h"
#include "config.h"
#include "dataStructures/smallMap.h"

#include <list>

class Area;
struct FarmField;
struct DeserializationMemo;
struct Faction;

struct FarmField
{
	SmallSet<Point3D> points;
	PlantSpeciesId plantSpecies;
	bool timeToSow = false;
	FarmField() = default;
	FarmField(FarmField&& other) noexcept = default;
	FarmField(SmallSet<Point3D>&& b) : points(std::move(b)) { }
	FarmField(const Json& data) { nlohmann::from_json(data, *this); }
	FarmField& operator=(FarmField&& other) noexcept = default;
	[[nodiscard]] bool operator==(const FarmField& other) const { return this == &other; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(FarmField, points, plantSpecies, timeToSow);
};
// To be used by HasFarmFields, which is used by Area.
class HasFarmFieldsForFaction
{
	FactionId m_faction;
	SmallSetStable<FarmField> m_farmFields;
	SmallSet<Point3D> m_pointsWithPlantsNeedingFluid;
	SmallSet<Point3D> m_pointsWithPlantsToHarvest;
	SmallSet<Point3D> m_pointsNeedingSeedsSewn;
	bool m_plantsNeedingFluidIsSorted = false;
public:
	HasFarmFieldsForFaction() = default;
	HasFarmFieldsForFaction(const FactionId& f) : m_faction(f) { }
	HasFarmFieldsForFaction(HasFarmFieldsForFaction&& other) noexcept = default;
	HasFarmFieldsForFaction& operator=(HasFarmFieldsForFaction&& other) noexcept = default;
	void addGivePlantFluidDesignation(Area& area, const Point3D& location);
	void removeGivePlantFluidDesignation(Area& area, const Point3D& location);
	void addSowSeedsDesignation(Area& area, const Point3D& point);
	void removeSowSeedsDesignation(Area& area, const Point3D& point);
	void addHarvestDesignation(Area& area, const PlantIndex& plant);
	void removeHarvestDesignation(Area& area, const PlantIndex& plant);
	void setDayOfYear(Area& area, uint32_t dayOfYear);
	[[nodiscard]] FarmField& create(Area& area, SmallSet<Point3D>&& space);
	[[nodiscard]] FarmField& create(Area& area, const SmallSet<Point3D>& space);
	[[nodiscard]] FarmField& create(Area& area, const Cuboid& cuboid);
	[[nodiscard]] FarmField& create(Area& area, const CuboidSet& cuboid);
	void extend(Area& area, FarmField& farmField, SmallSet<Point3D>& space);
	void setSpecies(Area& area, FarmField& farmField, const PlantSpeciesId& plantSpecies);
	void clearSpecies(Area& area, FarmField& farmField);
	void designatePoints(Area& area, FarmField& farmField, SmallSet<Point3D>& space);
	void shrink(Area& area, FarmField& farmField, SmallSet<Point3D>& space);
	void remove(Area& area, FarmField& farmField);
	void undesignatePoints(Area& area, SmallSet<Point3D>& space);
	void setPointData(Area& area);
	[[nodiscard]] PlantIndex getHighestPriorityPlantForGiveFluid(Area& area);
	[[nodiscard]] bool hasHarvestDesignations() const;
	[[nodiscard]] bool hasGivePlantsFluidDesignations() const;
	[[nodiscard]] bool hasSowSeedsDesignations() const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const Point3D& point) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasFarmFieldsForFaction, m_faction, m_farmFields, m_pointsWithPlantsNeedingFluid, m_pointsWithPlantsToHarvest, m_pointsNeedingSeedsSewn, m_plantsNeedingFluidIsSorted);
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
	void removeAllSowSeedsDesignations(const Point3D& point);
	void setDayOfYear(uint32_t dayOfYear);
	[[nodiscard]] bool hasGivePlantsFluidDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasHarvestDesignations(const FactionId& faction) const;
	[[nodiscard]] bool hasSowSeedsDesignations(const FactionId& faction) const;
	[[nodiscard]] PlantSpeciesId getPlantSpeciesFor(const FactionId& faction, const Point3D& location) const;
	// For testing.
	[[maybe_unused, nodiscard]] bool contains(const FactionId& faction) { return m_data.contains(faction); }
};
