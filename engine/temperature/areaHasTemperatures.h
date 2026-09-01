#pragma once
#include "temperatureSource.h"
#include "portals.h"

class FluidGroup;
class PointFeature;

// One per material type.
struct OnSurfaceData
{
	CuboidSet solid;
	CuboidSet features;
	CuboidSet items;
	bool empty() const { return solid.empty() && features.empty() && items.empty(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(OnSurfaceData, solid, features, items);
};
struct AreaHasTemperature
{
	AreaHasPortalsBetweenOutSideAndInside m_portals;
	AreaHasTemperatureSources m_sources;
	SmallMap<MaterialTypeId, OnSurfaceData> m_meltableMaterialTypeOnSurface;
	SmallMap<FluidTypeId, SmallSet<FluidGroupId>> m_freezableFluidTypeOnSurface;
	CuboidSet m_toUpdate;
	Temperature m_ambiant;
	Temperature m_maxAmbiant;
	Temperature m_minAmbiant;
	void markToUpdate(const CuboidSet& cuboids);
	void markToUpdate(Cuboid cuboid);
	void doStep(Area& area);
	void updateAmbientSurfaceTemperature(Area& area);
	void setAmbient(Area& area, const Temperature newAmbiant);
	void onTemperatureCanNoLongerTransmit(Area& area, const CuboidSet& cuboids);
	void onTemperatureCanNowTransmit(Area& area, const CuboidSet& cuboids);
	void onSetSolid(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType);
	void onSetNotSolid(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType);
	void onSetFeature(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType);
	void onUnsetFeature(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType);
	void onFluidEnters(Area& area, const CuboidSet& cuboids, FluidGroup& group);
	void onFluidExits(Area& area, const CuboidSet& cuboids, FluidTypeId type, FluidGroupId group);
	void onItemEnters(Area& area, ItemIndex item);
	void onItemExits(Area& area, ItemIndex item);
	void maybeRemoveFreezeableFluidGroupAboveGround(FluidTypeId type, FluidGroupId group);
	void addItemAboveGround(Area& area, ItemIndex item);
	void removeItemAboveGround(Area& area, ItemIndex item);
	void afterLoad(Area& area);
	void doPhaseChangeAndIgnition(Area& area);
	// Current temperature.
	[[nodiscard]] Temperature get(Area& area, Point3D point);
	[[nodiscard]] Temperature getDailyAverageAmbientSurfaceTemperature(Area& area) const;
	// Find the lowest temperature that might exist in cuboid. Does not guarantee that it does exist.
	[[nodiscard]] Temperature lowerBound(Area& area, Cuboid cuboid);
	[[nodiscard]] std::pair<Temperature, Temperature> upperAndLowerBounds(Area& area, Cuboid cuboid) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasTemperature, m_portals, m_sources, m_meltableMaterialTypeOnSurface, m_toUpdate, m_ambiant);
};