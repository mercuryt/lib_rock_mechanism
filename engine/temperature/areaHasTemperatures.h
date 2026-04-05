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
struct AreaHasTemperature2
{
	AreaHasPortalsBetweenOutSideAndInside m_portals;
	AreaHasTemperatureSources m_sources;
	SmallMap<MaterialTypeId, OnSurfaceData> m_meltableMaterialTypeOnSurface;
	SmallMap<FluidTypeId, SmallSet<FluidGroup*>> m_freezableFluidTypeOnSurface;
	CuboidSet m_toUpdate;
	Temperature m_ambiant;
	void markToUpdate(const CuboidSet& cuboids);
	void markToUpdate(const Cuboid cuboid);
	void doStep(Area& area);
	void updateAmbientSurfaceTemperature(Area& area);
	void setAmbient(Area& area, const Temperature newAmbiant);
	void onTemperatureCanNoLongerTransmit(Area& area, const CuboidSet& cuboids);
	void onTemperatureCanNowTransmit(Area& area, const CuboidSet& cuboids);
	void onSetSolid(Area& area, const CuboidSet& cuboids, const MaterialTypeId materialType);
	void onSetNotSolid(Area& area, const CuboidSet& cuboids, const MaterialTypeId materialType);
	void onSetFeature(Area& area, const Point3D point, const MaterialTypeId materialType);
	void onUnsetFeature(const Point3D point, const MaterialTypeId materialType);
	void onFluidEnters(Area& area, const CuboidSet& cuobids, FluidGroup& group);
	void onFluidExits(Area& area, const CuboidSet& cuobids, FluidGroup& group);
	void maybeRemoveFreezeableFluidGroupAboveGround(FluidGroup& group);
	void addItemAboveGround(Area& area, const ItemIndex item);
	void removeItemAboveGround(Area& area, const ItemIndex item);
	void afterLoad(Area& area);
	// Current temperature.
	[[nodiscard]] Temperature get(Area& area, const Point3D point);
	[[nodiscard]] Temperature getDailyAverageAmbientSurfaceTemperature(Area& area) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasTemperature2, m_portals, m_sources, m_meltableMaterialTypeOnSurface, m_toUpdate, m_ambiant);
};