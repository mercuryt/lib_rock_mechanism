#include "areaHasTemperatures.h"
#include "../definitions/materialType.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../fluidGroups/fluidGroup.h"
#include "../config/physics.h"
void AreaHasTemperature2::markToUpdate(const CuboidSet& cuboids) { m_toUpdate.maybeAddAll(cuboids); }
void AreaHasTemperature2::markToUpdate(const Cuboid cuboid) { m_toUpdate.maybeAdd(cuboid); }
void AreaHasTemperature2::doStep(Area& area)
{
	m_portals.doStep(area);
	m_sources.doStep(area);
	if(m_toUpdate.empty())
		return;
	Space& space = area.getSpace();
	// Actors.
	Actors& actors = area.getActors();
	const SmallSet<ActorIndex> actorsInArea = space.actor_getAll(m_toUpdate);
	for(const ActorIndex actor : actorsInArea)
		actors.temperature_onChange(actor);
	Plants& plants = area.getPlants();
	// Convert plants into locations for stability.
	SmallSet<Point3D> plantsInArea;
	space.plant_queryForEach(m_toUpdate, [&](const PlantIndex& plant){
		plantsInArea.maybeInsert(plants.getLocation(plant));
	});
	for(const Point3D plantLocation : plantsInArea)
	{
		PlantIndex plant = space.plant_get(plantLocation);
		plants.setTemperature(plant, get(area, plantLocation));
	}
	// Melt or burn items.
	const SmallSet<ItemIndex> itemIndices = space.item_getAll(m_toUpdate);
	// Items may be destroyed by melting so store as references.
	SmallSet<ItemReference> itemsInArea;
	itemsInArea.reserve(itemIndices.size());
	const int end = itemIndices.size();
	Items& items = area.getItems();
	for(int i = 0; i < end; ++i)
		itemsInArea.insert(items.getReference(itemIndices[i]));
	for(const ItemReference ref : itemsInArea)
	{
		const ItemIndex item = ref.getIndex(items.m_referenceData);
		const Point3D location = items.getLocation(item);
		items.setTemperature(item, get(area, location), location);
	}
	// Fluids may freeze.
	SmallMap<FluidTypeId, CuboidSet> m_toFreeze;
	space.fluid_queryForEachWithCuboids(m_toUpdate, [&](const Cuboid cuboid, const FluidData fluidData){
		const Temperature freezingPoint = FluidType::getFreezingPoint(fluidData.type);
		const CuboidSet intersection = m_toUpdate.intersection(cuboid);
		CuboidSet toFreezeInCuboid;
		for(const Cuboid intersectionCuboid : intersection)
			for(const Point3D point : intersectionCuboid)
			{
				const Temperature temperature = get(area, point);
				if(temperature <= freezingPoint)
					toFreezeInCuboid.add(point);
			}
		if(toFreezeInCuboid.exists())
			m_toFreeze.getOrCreate(fluidData.type).addAll(toFreezeInCuboid);
	});
	for(const auto& [fluidType, cuboidSet] : m_toFreeze)
		for(const Cuboid cuboid : cuboidSet)
			for(const Point3D point : cuboid)
				space.temperature_freeze(point, fluidType);
	// Melt or burn features.
	SmallMap<MaterialTypeId, CuboidSet> toMeltFeatures;
	SmallMap<MaterialTypeId, CuboidSet> toBurnFeatures;
	space.pointFeature_queryForEachWithCuboids(m_toUpdate, [&](const Cuboid cuboid, const PointFeature pointFeature){
		Temperature meltingPoint = MaterialType::getMeltingPoint(pointFeature.materialType);
		Temperature ignitionPoint = MaterialType::getIgnitionTemperature(pointFeature.materialType);
		if(meltingPoint.empty() && ignitionPoint.empty())
			return;
		CuboidSet pointsOfCuboidToMelt;
		CuboidSet pointsOfCuboidToBurn;
		for(const Point3D point : cuboid)
		{
			Temperature temperature = get(area, point);
			if(meltingPoint.exists() && meltingPoint <= temperature)
				pointsOfCuboidToMelt.add(point);
			else if(ignitionPoint.exists() && ignitionPoint <= temperature)
				pointsOfCuboidToBurn.add(point);
		}
		if(pointsOfCuboidToMelt.exists())
			toMeltFeatures.getOrCreate(pointFeature.materialType).addAll(pointsOfCuboidToMelt);
		if(pointsOfCuboidToBurn.exists())
			toBurnFeatures.getOrCreate(pointFeature.materialType).addAll(pointsOfCuboidToBurn);
	});
	for(const auto& [materialType, cuboids] : toMeltFeatures)
		space.temperature_meltFeatures(cuboids, materialType);
	for(const auto& [materialType, cuboids] : toBurnFeatures)
		for(const Cuboid cuboid : cuboids)
			for(const Point3D point : cuboid)
				area.m_fires.ignite(area, point, materialType);
	// Melt or burn solid.
	SmallMap<MaterialTypeId, CuboidSet> toMeltSolid;
	SmallMap<MaterialTypeId, CuboidSet> toBurnSolid;
	space.solid_queryForEachWithCuboids(m_toUpdate, [&](const Cuboid cuboid, const MaterialTypeId materialType){
		const Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
		const Temperature ignitionPoint = MaterialType::getIgnitionTemperature(materialType);
		if(meltingPoint.empty() && ignitionPoint.empty())
			return;
		CuboidSet pointsOfCuboidToMelt;
		CuboidSet pointsOfCuboidToBurn;
		CuboidSet pointsOfCuboidToUpdate = m_toUpdate.intersection(cuboid);
		for(const Cuboid cuboidToUpdate : pointsOfCuboidToUpdate)
			for(const Point3D point : cuboidToUpdate)
			{
				Temperature temperature = get(area, point);
				if(!meltingPoint.empty() && meltingPoint <= temperature)
					pointsOfCuboidToMelt.add(point);
				else if(!ignitionPoint.empty() && ignitionPoint <= temperature && !space.fire_exists(point))
					pointsOfCuboidToBurn.add(point);
			}
		if(pointsOfCuboidToMelt.exists())
			toMeltSolid.getOrCreate(materialType).addAll(pointsOfCuboidToMelt);
		if(pointsOfCuboidToBurn.exists())
			toBurnSolid.getOrCreate(materialType).addAll(pointsOfCuboidToBurn);
	});
	for(const auto& [materialType, cuboids] : toMeltSolid)
		space.temperature_meltSolid(cuboids, materialType);
	for(const auto& [materialType, cuboids] : toBurnSolid)
		for(const Cuboid cuboid : cuboids)
			for(const Point3D point : cuboid)
				area.m_fires.ignite(area, point, materialType);
	m_toUpdate.clear();
}
void AreaHasTemperature2::setAmbient(Area& area, const Temperature newAmbiant)
{
	m_ambiant = newAmbiant;
	// Ignite is skipped, it is assumed that sunlight cannot cause ignition.
	Space& space = area.getSpace();
	// Collect space in range of a temperature source, do not proccess these, they will be handled seperately.
	CuboidSet inRangeOfSource = m_sources.onChangeAmbiantSurfaceTemperatureReturnIntersection(area);
	// Freeze.
	// Create a copy to modifiy
	for(auto [fluidType, groups] : m_freezableFluidTypeOnSurface)
	{
		Temperature freezingPoint = FluidType::getFreezingPoint(fluidType);
		if(freezingPoint < newAmbiant)
			continue;
		CuboidSet toFreeze;
		for(const FluidGroup* group : groups)
			toFreeze.maybeAddAll(group->getPoints());
		for(Cuboid& cuboid : toFreeze)
			if(cuboid.sizeZ() > 2)
				cuboid.m_low.setZ(std::max(cuboid.m_low.z(), cuboid.m_high.z() - 1));
		toFreeze = space.m_exposedToSky.get().queryGetIntersection(toFreeze);
		if(toFreeze.exists())
			space.temperature_freeze(toFreeze, fluidType);
	}
	area.getPlants().onChangeAmbiantSurfaceTemperature(newAmbiant, inRangeOfSource);
	area.getActors().onChangeAmbiantSurfaceTemperature(newAmbiant, inRangeOfSource);
	// Melt.
	// Create a copy to modifiy
	for(auto [materialType, onSurfaceData] : m_meltableMaterialTypeOnSurface)
	{
		Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
		if(meltingPoint.exists() && meltingPoint > newAmbiant)
			continue;
		onSurfaceData.items.maybeRemoveAll(inRangeOfSource);
		if(onSurfaceData.items.exists())
			space.temperature_meltItems(onSurfaceData.items, materialType);
		onSurfaceData.features.maybeRemoveAll(inRangeOfSource);
		if(onSurfaceData.features.exists())
			space.temperature_meltFeatures(onSurfaceData.features, materialType);
		onSurfaceData.solid.maybeRemoveAll(inRangeOfSource);
		if(onSurfaceData.solid.exists())
			space.temperature_meltSolid(onSurfaceData.solid, materialType);
	}
}
void AreaHasTemperature2::onTemperatureCanNoLongerTransmit(Area& area, const CuboidSet& cuboids)
{
	m_portals.onTemperatureCanNoLongerTransmit(area, cuboids);
	m_sources.onTemperatureCanNoLongerTransmit(cuboids);
}
void AreaHasTemperature2::onTemperatureCanNowTransmit(Area& area, const CuboidSet& cuboids)
{
	m_portals.onTemperatureCanNowTransmit(area, cuboids);
	m_sources.onTemperatureCanNowTransmit(cuboids);
}
Temperature AreaHasTemperature2::get(Area& area,const Point3D point)
{
	Space& space = area.getSpace();
	bool isExposedToSky = space.m_exposedToSky.check(point);
	Temperature ambiant = isExposedToSky ?
		m_ambiant :
		Config::undergroundAmbiantTemperature;
	return ambiant + m_sources.getDelta(point) + m_portals.getDelta(area, point);
}
void AreaHasTemperature2::onSetSolid(Area& area, const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	Space& space = area.getSpace();
	onTemperatureCanNoLongerTransmit(area, cuboids);
	if(MaterialType::canMelt(materialType))
	{
		CuboidSet intersection = space.m_exposedToSky.get().queryGetIntersection(cuboids);
		if(!intersection.empty())
			m_meltableMaterialTypeOnSurface.getOrCreate(materialType).solid.maybeAdd(intersection);
	}
}
void AreaHasTemperature2::onSetNotSolid(Area& area, const CuboidSet& cuboids, const MaterialTypeId materialType)
{
	onTemperatureCanNowTransmit(area, cuboids);
	if(MaterialType::canMelt(materialType))
	{
		auto found = m_meltableMaterialTypeOnSurface.find(materialType);
		if(found != m_meltableMaterialTypeOnSurface.end())
			found->second.solid.maybeRemoveAll(cuboids);
	}
	// Gather space underneath to add to onSurface data.
}
void AreaHasTemperature2::onSetFeature(Area& area, const Point3D point, const MaterialTypeId materialType)
{
	if(MaterialType::canMelt(materialType))
	{
		Space& space = area.getSpace();
		if(space.m_exposedToSky.check(point))
			m_meltableMaterialTypeOnSurface.getOrCreate(materialType).features.maybeAdd(point);
	}
}
void AreaHasTemperature2::onUnsetFeature(const Point3D point, const MaterialTypeId materialType)
{
	// Count features with material type at point.
	auto found = m_meltableMaterialTypeOnSurface.find(materialType);
	if(found != m_meltableMaterialTypeOnSurface.end())
		found->second.features.maybeRemove(point);
}
void AreaHasTemperature2::onFluidEnters(Area& area, const CuboidSet& cuboids, FluidGroup& group)
{
	if(FluidType::canFreeze(group.m_fluidType))
	{
		// TODO: profile this branch.
		if(m_freezableFluidTypeOnSurface.contains(group.m_fluidType) && m_freezableFluidTypeOnSurface[group.m_fluidType].contains(&group))
			return;
		if(area.getSpace().m_exposedToSky.get().query(cuboids))
			m_freezableFluidTypeOnSurface.getOrCreate(group.m_fluidType).insert(&group);
	}
}
void AreaHasTemperature2::onFluidExits(Area& area, const CuboidSet& cuboids, FluidGroup& group)
{
	if(FluidType::canFreeze(group.m_fluidType))
	{
		if(!m_freezableFluidTypeOnSurface.contains(group.m_fluidType) || !m_freezableFluidTypeOnSurface[group.m_fluidType].contains(&group))
			return;
		if(!area.getSpace().m_exposedToSky.get().query(cuboids))
			return;
		if(!area.getSpace().m_exposedToSky.get().query(group.getPoints()))
			m_freezableFluidTypeOnSurface[group.m_fluidType].erase(&group);
	}
}
void AreaHasTemperature2::maybeRemoveFreezeableFluidGroupAboveGround(FluidGroup& group)
{
	if(!m_freezableFluidTypeOnSurface.contains(group.m_fluidType))
		return;
	m_freezableFluidTypeOnSurface[group.m_fluidType].maybeErase(&group);
}
void AreaHasTemperature2::addItemAboveGround(Area& area, const ItemIndex item)
{
	Items& items = area.getItems();
	MaterialTypeId materialType = items.getMaterialType(item);
	if(materialType.empty())
		materialType = items.getConstructedShape(item).getMaterialWithTheLowestMeltingPoint();
	// If the constructed shape has no meltable material types then material type will still by empty.
	if(materialType.exists() && MaterialType::canMelt(materialType))
		m_meltableMaterialTypeOnSurface.getOrCreate(materialType).items.maybeAddAll(items.getOccupied(item));
}
void AreaHasTemperature2::removeItemAboveGround(Area& area, const ItemIndex item)
{
	Items& items = area.getItems();
	MaterialTypeId materialType = items.getMaterialType(item);
	if(materialType.empty())
		materialType = items.getConstructedShape(item).getMaterialWithTheLowestMeltingPoint();
	if(materialType.exists() && MaterialType::canMelt(materialType))
	{
		auto found = m_meltableMaterialTypeOnSurface.find(items.getMaterialType(item));
		if(found == m_meltableMaterialTypeOnSurface.end())
			return;
		found->second.items.maybeRemoveAll(items.getOccupied(item));
	}
}
void AreaHasTemperature2::afterLoad(Area& area)
{
	Space& space = area.getSpace();
	space.m_exposedToSky.get().forEach([&](const Cuboid exposedCuboid){
		space.fluid_queryForEach(exposedCuboid, [&](const FluidData data){ m_freezableFluidTypeOnSurface.getOrCreate(data.type).maybeInsert(data.group); });
	});
}
void AreaHasTemperature2::updateAmbientSurfaceTemperature(Area& area)
{
	// TODO: Latitude and altitude.
	Temperature dailyAverage = getDailyAverageAmbientSurfaceTemperature(area);
	static Temperature maxDailySwing = Temperature::create(35);
	static int hottestHourOfDay = 14;
	int hour = DateTime(area.m_simulation.m_step).hour;
	int hoursFromHottestHourOfDay = std::abs((int)hottestHourOfDay - hour);
	int halfDay = Config::hoursPerDay / 2;
	setAmbient(area, dailyAverage + ((maxDailySwing * (std::max(0, halfDay - hoursFromHottestHourOfDay))) / halfDay) - (maxDailySwing / 2));
}
Temperature AreaHasTemperature2::getDailyAverageAmbientSurfaceTemperature(Area& area) const
{
	// TODO: Latitude and altitude.
	static Temperature yearlyHottestDailyAverage = Temperature::create(290);
	static Temperature yearlyColdestDailyAverage = Temperature::create(270);
	static int dayOfYearOfSolstice = Config::daysPerYear / 2;
	int day = DateTime(area.m_simulation.m_step).day;
	int daysFromSolstice = std::abs(day - (int)dayOfYearOfSolstice);
	return yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
}