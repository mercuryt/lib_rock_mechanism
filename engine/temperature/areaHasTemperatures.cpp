#include "areaHasTemperatures.h"
#include "../definitions/materialType.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../plants.h"
#include "../fluid/fluidGroup.h"
#include "../config/physics.h"
#include "../geometry/cuboidSetHelper.hpp"
void AreaHasTemperature::markToUpdate(const CuboidSet& cuboids) { m_toUpdate.maybeAddAll(cuboids); }
void AreaHasTemperature::markToUpdate(Cuboid cuboid) { m_toUpdate.maybeAdd(cuboid); }
void AreaHasTemperature::doStep(Area& area)
{
	m_portals.doStep(area);
	m_sources.doStep(area);
	if(m_toUpdate.empty())
		return;
	Space& space = area.getSpace();
	// Actors.
	Actors& actors = area.getActors();
	const SmallSet<ActorIndex> actorsInArea = space.actor_getAll(m_toUpdate);
	for(ActorIndex actor : actorsInArea)
		actors.temperature_onChange(actor);
	Plants& plants = area.getPlants();
	// Convert plants into locations for stability.
	SmallSet<Point3D> plantsInArea;
	space.plant_queryForEach(m_toUpdate, [&](const PlantIndex& plant){
		plantsInArea.maybeInsert(plants.getLocation(plant));
	});
	for(Point3D plantLocation : plantsInArea)
	{
		PlantIndex plant = space.plant_get(plantLocation);
		plants.setTemperature(plant, get(area, plantLocation));
	}
	doPhaseChangeAndIgnition(area);
	m_toUpdate.clear();
}
void AreaHasTemperature::setAmbient(Area& area, const Temperature newAmbiant)
{
	m_ambiant = newAmbiant;
	// Ignite is skipped, it is assumed that sunlight cannot cause ignition.
	Space& space = area.getSpace();
	// Collect space in range of a temperature source, do not proccess these, they will be handled seperately.
	CuboidSet inRangeOfSource = m_sources.onChangeAmbiantSurfaceTemperatureReturnIntersection(area);
	// Freeze.
	for(auto& [fluidType, groups] : m_freezableFluidTypeOnSurface)
	{
		Temperature freezingPoint = FluidType::getFreezingPoint(fluidType);
		if(freezingPoint < newAmbiant)
			continue;
		CuboidSet toFreeze;
		for(const FluidGroupId groupId : groups)
		{
			FluidGroup& group = area.m_hasFluidGroups.byId(groupId);
			toFreeze.maybeAddAll(group.m_occupied);
		}
		for(Cuboid& cuboid : toFreeze)
			if(cuboid.sizeZ() > 2)
				cuboid.m_low.setZ(std::max(cuboid.m_low.z(), cuboid.m_high.z() - 1));
		toFreeze = space.m_exposedToSky.get().queryGetIntersection(toFreeze);
		if(toFreeze.exists())
			area.m_hasPhaseChanges.setFreezing(fluidType, toFreeze);
	}
	area.getPlants().onChangeAmbiantSurfaceTemperature(newAmbiant, inRangeOfSource);
	area.getActors().onChangeAmbiantSurfaceTemperature(newAmbiant, inRangeOfSource);
	// Melt.
	for(auto& [materialType, onSurfaceData] : m_meltableMaterialTypeOnSurface)
	{
		Temperature meltingPoint = MaterialType::getMeltingPoint(materialType);
		if(meltingPoint.exists() && meltingPoint > newAmbiant)
			continue;
		CuboidSet toMelt;
		assert(!(onSurfaceData.solid.empty() && onSurfaceData.features.empty() && onSurfaceData.items.empty()));
		toMelt.maybeAdd(onSurfaceData.solid);
		toMelt.maybeAdd(onSurfaceData.features);
		toMelt.maybeAdd(onSurfaceData.items);
		if(toMelt.exists())
			area.m_hasPhaseChanges.setMelting(materialType, toMelt);
	}
}
void AreaHasTemperature::onTemperatureCanNoLongerTransmit(Area& area, const CuboidSet& cuboids)
{
	m_portals.onTemperatureCanNoLongerTransmit(area, cuboids);
	m_sources.onTemperatureCanNoLongerTransmit(cuboids);
}
void AreaHasTemperature::onTemperatureCanNowTransmit(Area& area, const CuboidSet& cuboids)
{
	m_portals.onTemperatureCanNowTransmit(area, cuboids);
	m_sources.onTemperatureCanNowTransmit(cuboids);
}
Temperature AreaHasTemperature::get(Area& area,Point3D point)
{
	Space& space = area.getSpace();
	bool isExposedToSky = space.m_exposedToSky.check(point);
	Temperature ambiant = isExposedToSky ?
		m_ambiant :
		Config::undergroundAmbiantTemperature;
	return ambiant + m_sources.getDelta(point) + m_portals.getDelta(area, point);
}
void AreaHasTemperature::onSetSolid(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType)
{
	Space& space = area.getSpace();
	onTemperatureCanNoLongerTransmit(area, cuboids);
	area.m_hasPhaseChanges.onSolidSet(area, materialType, cuboids);
	if(MaterialType::canMelt(materialType))
	{
		CuboidSet intersection = space.m_exposedToSky.get().queryGetIntersection(cuboids);
		if(!intersection.empty())
			m_meltableMaterialTypeOnSurface.getOrCreate(materialType).solid.maybeAdd(intersection);
	}
}
void AreaHasTemperature::onSetNotSolid(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType)
{
	onTemperatureCanNowTransmit(area, cuboids);
	area.m_hasPhaseChanges.onSolidSetNot(materialType, cuboids);
	if(MaterialType::canMelt(materialType))
	{
		auto found = m_meltableMaterialTypeOnSurface.find(materialType);
		if(found != m_meltableMaterialTypeOnSurface.end())
			found->second.solid.maybeRemoveAll(cuboids);
	}
	// Gather space underneath to add to onSurface data.
}
void AreaHasTemperature::onSetFeature(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType)
{
	area.m_hasPhaseChanges.onFeatureSet(area, materialType, cuboids);
	if(MaterialType::canMelt(materialType))
	{
		Space& space = area.getSpace();
		CuboidSet exposedToSky = space.m_exposedToSky.get().queryGetIntersection(cuboids);
		m_meltableMaterialTypeOnSurface.getOrCreate(materialType).features.maybeAdd(exposedToSky);
	}
}
void AreaHasTemperature::onUnsetFeature(Area& area, const CuboidSet& cuboids, MaterialTypeId materialType)
{
	area.m_hasPhaseChanges.onFeatureSetNot(area, materialType, cuboids);
	// Count features with material type at point.
	auto found = m_meltableMaterialTypeOnSurface.find(materialType);
	if(found != m_meltableMaterialTypeOnSurface.end())
		found->second.features.maybeRemove(cuboids);
}
void AreaHasTemperature::onFluidEnters(Area& area, const CuboidSet& cuboids, FluidGroup& group)
{
	Space& space = area.getSpace();
	if(!group.m_aboveGround && space.m_exposedToSky.check(cuboids))
		group.m_aboveGround = true;
	if(FluidType::canFreeze(group.m_fluidType))
	{
		area.m_hasPhaseChanges.onFluidEnters(area, group.m_fluidType, cuboids);
		// TODO: profile this branch.
		if(m_freezableFluidTypeOnSurface.contains(group.m_fluidType) && m_freezableFluidTypeOnSurface[group.m_fluidType].contains(group.m_id))
			return;
		if(area.getSpace().m_exposedToSky.get().query(cuboids))
			m_freezableFluidTypeOnSurface.getOrCreate(group.m_fluidType).insert(group.m_id);
	}
}
void AreaHasTemperature::onFluidExits(Area& area, const CuboidSet& cuboids, FluidTypeId fluidType, FluidGroupId group)
{
	area.m_hasPhaseChanges.onFluidExits(fluidType, cuboids);
	if(FluidType::canFreeze(fluidType))
	{
		if(!m_freezableFluidTypeOnSurface.contains(fluidType) || !m_freezableFluidTypeOnSurface[fluidType].contains(group))
			return;
		if(!area.getSpace().m_exposedToSky.get().query(cuboids))
			return;
		if(!area.getSpace().m_exposedToSky.get().query(area.m_hasFluidGroups.byId(group).m_occupied))
			m_freezableFluidTypeOnSurface[fluidType].erase(group);
	}
}
void AreaHasTemperature::onItemEnters(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	MaterialTypeId materialType = items.getMaterialType(item);
	// If materialType does not exist this is a constrcuted shape.
	// TODO: interaction between temperature and constructed shapes.
	if(materialType.exists())
		area.m_hasPhaseChanges.onItemEnter(area, materialType, items.getOccupied(item));
	if(!items.isOnSurface(item) && area.getSpace().m_exposedToSky.get().query(items.getOccupied(item)))
		items.setOnSurface(item, true);
}
void AreaHasTemperature::onItemExits(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	MaterialTypeId materialType = items.getMaterialType(item);
	area.m_hasPhaseChanges.onItemExit(area, materialType, items.getOccupied(item));
	if(items.isOnSurface(item))
		items.setOnSurface(item, false);
}
void AreaHasTemperature::maybeRemoveFreezeableFluidGroupAboveGround(FluidTypeId fluidType, FluidGroupId group)
{
	if(!m_freezableFluidTypeOnSurface.contains(fluidType))
		return;
	m_freezableFluidTypeOnSurface[fluidType].maybeErase(group);
}
void AreaHasTemperature::addItemAboveGround(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	MaterialTypeId materialType = items.getMaterialType(item);
	if(materialType.empty())
		materialType = items.getConstructedShape(item).getMaterialWithTheLowestMeltingPoint();
	// If the constructed shape has no meltable material types then material type will still by empty.
	if(materialType.exists() && MaterialType::canMelt(materialType))
		m_meltableMaterialTypeOnSurface.getOrCreate(materialType).items.maybeAddAll(items.getOccupied(item));
}
void AreaHasTemperature::removeItemAboveGround(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	Space& space = area.getSpace();
	MaterialTypeId materialType = items.getMaterialType(item);
	if(materialType.empty())
		materialType = items.getConstructedShape(item).getMaterialWithTheLowestMeltingPoint();
	if(materialType.exists() && MaterialType::canMelt(materialType))
	{
		auto found = m_meltableMaterialTypeOnSurface.find(items.getMaterialType(item));
		if(found == m_meltableMaterialTypeOnSurface.end())
			return;
		CuboidSet toRemove = items.getOccupied(item);
		space.item_queryForEach(toRemove, [materialType, &toRemove, &items](ItemIndex itemIndex) mutable {
			if(items.getMaterialType(itemIndex) == materialType)
				toRemove.remove(items.getOccupied(itemIndex));
		});
		if(!toRemove.empty())
			found->second.items.maybeRemoveAll(toRemove);
	}
}
void AreaHasTemperature::afterLoad(Area& area)
{
	Space& space = area.getSpace();
	space.m_exposedToSky.get().forEach([&](Cuboid exposedCuboid){
		space.fluid_queryForEach(exposedCuboid, [&](const FluidData data){ m_freezableFluidTypeOnSurface.getOrCreate(data.type).maybeInsert(data.group); });
	});
}
void AreaHasTemperature::updateAmbientSurfaceTemperature(Area& area)
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
Temperature AreaHasTemperature::getDailyAverageAmbientSurfaceTemperature(Area& area) const
{
	Temperature yearlyHottestDailyAverage = m_maxAmbiant.exists() ? m_maxAmbiant : Temperature::create(290);
	Temperature yearlyColdestDailyAverage = m_minAmbiant.exists() ? m_minAmbiant : Temperature::create(270);;
	static int dayOfYearOfSolstice = Config::daysPerYear / 2;
	int day = DateTime(area.m_simulation.m_step).day;
	int daysFromSolstice = std::abs(day - (int)dayOfYearOfSolstice);
	return yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
}
Temperature AreaHasTemperature::lowerBound(Area& area, Cuboid cuboid)
{
	TemperatureDelta sumDelta{0};
	m_sources.queryForEach(cuboid, [&](const TemperatureSource source){
		Distance distance = source.m_delta > 0 ?
			// Delta is positive, distance is farthest possible.
			cuboid.furthestPointFrom(source.m_location).distanceTo(source.m_location) :
			// Delta is negitive, distance is closes possible.
			source.m_location.distanceTo(cuboid);
		TemperatureDelta delta = source.m_delta.reduceForDistanceRadiant(distance.toFloat());
		sumDelta += delta;
	});
	TemperatureDelta deltaBetweenExposedAndNotExposed = TemperatureDelta::create((m_ambiant - Config::undergroundAmbiantTemperature).get());
	m_portals.queryForEach(cuboid, [&](Cuboid portal){
		// Portals are always cooling sources rather then heat sources.
		Distance distance = portal.distanceTo(cuboid);
		TemperatureDelta delta = deltaBetweenExposedAndNotExposed.reduceForDistanceAmbiant(distance.toFloat());
		sumDelta += delta;
	});
	if(area.getSpace().m_exposedToSky.get().query(cuboid))
		// At least some part of cuboid is exposed to sky.
		return std::min(m_ambiant, Config::undergroundAmbiantTemperature) + sumDelta;
	return Config::undergroundAmbiantTemperature + sumDelta;
}
std::pair<Temperature, Temperature> AreaHasTemperature::upperAndLowerBounds(Area& area, Cuboid cuboid) const
{
	TemperatureDelta highDelta{0};
	TemperatureDelta lowDelta{0};
	m_sources.queryForEach(cuboid, [&](const TemperatureSource source){
		Distance distanceLow = source.m_delta > 0 ?
			// Delta is positive, distance is farthest possible.
			cuboid.furthestPointFrom(source.m_location).distanceTo(source.m_location) :
			// Delta is negitive, distance is closest possible.
			source.m_location.distanceTo(cuboid);
		TemperatureDelta deltaLow = source.m_delta.reduceForDistanceRadiant(distanceLow.toFloat());
		lowDelta += deltaLow;
		Distance distanceHigh = source.m_delta < 0 ?
			// Delta is positive, distance is closest possible.
			cuboid.furthestPointFrom(source.m_location).distanceTo(source.m_location) :
			// Delta is negitive, distance is farthest possible.
			source.m_location.distanceTo(cuboid);
		TemperatureDelta deltaHigh = source.m_delta.reduceForDistanceRadiant(distanceHigh.toFloat());
		highDelta += deltaHigh;
	});
	TemperatureDelta deltaBetweenExposedAndNotExposed = m_ambiant.delta() - Config::undergroundAmbiantTemperature.delta();
	m_portals.queryForEach(cuboid, [&](Cuboid portal){
		// Portals are always cooling sources rather then heat sources.
		Distance distance = portal.distanceTo(cuboid);
		TemperatureDelta delta = deltaBetweenExposedAndNotExposed.reduceForDistanceAmbiant(distance.toFloat());
		lowDelta += delta;
	});
	if(area.getSpace().m_exposedToSky.get().query(cuboid))
		// At least some part of cuboid is exposed to sky.
		return {
			std::max(m_ambiant, Config::undergroundAmbiantTemperature) + highDelta,
			std::min(m_ambiant, Config::undergroundAmbiantTemperature) + lowDelta
		};
	return {Config::undergroundAmbiantTemperature + highDelta, Config::undergroundAmbiantTemperature + lowDelta};
}
void AreaHasTemperature::doPhaseChangeAndIgnition(Area& area)
{
	// It would be nicer logically to seperate this into AreaHasFires::onTemperatureChange and AreaHasPhaseChanges::onTemperatureChange, but it would involve many repetitive boundry computations with the temperature source tree.
	auto [upperToUpdateBound, lowerToUpdateBound] = area.m_hasTemperature.upperAndLowerBounds(area, m_toUpdate.boundry());
	Space& space = area.getSpace();
	// Melt or ignite
	CuboidSet maybeStopMelting;
	CuboidSet maybeStartMelting;
	SmallMap<MaterialTypeId, CuboidSet> maybeStartOrStopMelting;
	CuboidSet igniteIfNotBurning;
	SmallMap<MaterialTypeId, CuboidSet> maybeIgniteIfNotBurning;
	// This query collects cuboids into catagories yes, no, maybe for each cuboid for both melting and iginition.
	// Note that the "maybe" in maybeStartMelting, etc. means set if not set already, it does not refer to temperature.
	space.solid_queryForEachWithCuboids(m_toUpdate, [upperToUpdateBound, lowerToUpdateBound, &maybeStopMelting, &maybeStartMelting, &maybeStartOrStopMelting, &igniteIfNotBurning, &maybeIgniteIfNotBurning](Cuboid cuboid, MaterialTypeId material){
		// Melting.
		Temperature meltingPoint = MaterialType::getMeltingPoint(material);
		if(meltingPoint.exists())
		{
			if(meltingPoint > upperToUpdateBound)
				// No point in the m_toUpdate of this materialType is melting.
				maybeStopMelting.add(cuboid);
			else if(meltingPoint < lowerToUpdateBound)
				// All points in the m_toUpdate are melting
				maybeStartMelting.add(cuboid);
			else
				// Some but not all are melting.
				maybeStartOrStopMelting.getOrCreate(material).add(cuboid);
		}
		// Ignition.
		Temperature ignitionPoint = MaterialType::getIgnitionTemperature(material);
		if(ignitionPoint.exists())
		{
			if(ignitionPoint > upperToUpdateBound)
			{
				// Do nothing
			}
			else if(ignitionPoint <= lowerToUpdateBound)
				// All points in m_toUpdate for this material type are iginiting.
				igniteIfNotBurning.add(cuboid);
			else
				// Some but not all are iginiting.
				maybeIgniteIfNotBurning.getOrCreate(material).add(cuboid);
		}
	});
	// Categorize the remainder using cuboidSetHelper::queryReturnTrueAndFalse to categorize the cuboids not categorized in the inital broad phase.
	// These will be subdivided and/or examined point-by-point as needed.
	// TODO: is there a redundant temperature bounds check at the begining of queryReturnTrueAndFalse?
	// First melting.
	for(auto& [material, cuboidSet] : maybeStartOrStopMelting)
	{
		Temperature meltingPoint = MaterialType::getMeltingPoint(material);
		auto cuboidCondition = [meltingPoint, &area](Cuboid cuboid) -> std::optional<bool> {
			auto [upper, lower] = area.m_hasTemperature.upperAndLowerBounds(area, cuboid);
			// If the highest temperature that could exist in the cuboid is less then the melting point then none of it melts.
			if(upper < meltingPoint)
				return {false};
			// If the lowest temperature that could exist in the cuboid is greater then or equal to melting point then all of it melts.
			if(lower >= meltingPoint)
				return {true};
			// Some but not all of the cuboid melts.
			return std::optional<bool>{};
		};
		auto pointCondition = [meltingPoint, &area](Point3D point) -> bool {
			return meltingPoint <= area.m_hasTemperature.get(area, point);
		};
		auto [toMelt, toNotMelt] = cuboidSetHelper::queryReturnTrueAndFalse(cuboidSet, cuboidCondition, pointCondition);
		if(toMelt.exists())
			area.m_hasPhaseChanges.m_melting.getOrCreate(material).maybeAdd(toMelt);
		if(toNotMelt.exists())
		{
			auto found = area.m_hasPhaseChanges.m_melting.find(material);
			if(found != area.m_hasPhaseChanges.m_melting.end())
				found->second.maybeRemove(toNotMelt);
		}
	}
	// Second ignition.
	for(auto& [material, cuboidSet] : maybeIgniteIfNotBurning)
	{
		Temperature ignitionPoint = MaterialType::getIgnitionTemperature(material);
		auto cuboidCondition = [ignitionPoint, &area](Cuboid cuboid) -> std::optional<bool> {
			auto [upper, lower] = area.m_hasTemperature.upperAndLowerBounds(area, cuboid);
			// If the highest temperature that could exist in the cuboid is less then the ignition point then none of it ignites.
			if(upper < ignitionPoint)
				return {false};
			// If the lowest temperature that could exist in the cuboid is greater then or equal to the ignition point then all of it ignites.
			if(lower >= ignitionPoint)
				return {true};
			// Some but not all of the cuboid ignites.
			return std::optional<bool>{};
		};
		auto pointCondition = [ignitionPoint, &area](Point3D point) -> bool {
			return ignitionPoint <= area.m_hasTemperature.get(area, point);
		};
		// No toNotIgnite here: ignition cannot be reversed.
		auto toIgnite = cuboidSetHelper::query(cuboidSet, cuboidCondition, pointCondition);
		if(toIgnite.exists())
			area.m_fires.ignite(area, toIgnite, material);
	}
	// Freeze.
	CuboidSet maybeStopFreezing;
	CuboidSet maybeStartFreezing;
	SmallMap<FluidTypeId, CuboidSet> maybeStartOrStopFreezing;
	space.fluid_queryForEachWithCuboids(m_toUpdate,
		[upperToUpdateBound, lowerToUpdateBound, &maybeStopFreezing, &maybeStartFreezing, &maybeStartOrStopFreezing]
		(Cuboid cuboid, FluidData fluid){
			Temperature freezingPoint = FluidType::getFreezingPoint(fluid.type);
			if(freezingPoint.empty())
				return;
			if(freezingPoint > upperToUpdateBound)
				// No point in the m_toUpdate of this materialType is freezeing.
				maybeStopFreezing.add(cuboid);
			else if(freezingPoint < lowerToUpdateBound)
				// All points in the m_toUpdate are freezeing
				maybeStartFreezing.add(cuboid);
			else
				// Some but not all are freezeing.
				maybeStartOrStopFreezing.getOrCreate(fluid.type).add(cuboid);
		}
	);
	for(auto& [fluid, cuboidSet] : maybeStartOrStopFreezing)
	{
		Temperature freezeingPoint = FluidType::getFreezingPoint(fluid);
		auto cuboidCondition = [freezeingPoint, &area](Cuboid cuboid) -> std::optional<bool> {
			auto [upper, lower] = area.m_hasTemperature.upperAndLowerBounds(area, cuboid);
			// If the highest temperature that could exist in the cuboid is less then or equal to the freezeing point then all of it freezes.
			if(upper <= freezeingPoint)
				return {true};
			// If the lowest temperature that could exist in the cuboid is greater then freezeing point then none of it freezes.
			if(lower > freezeingPoint)
				return {false};
			// Some but not all of the cuboid freezes.
			return std::optional<bool>{};
		};
		auto pointCondition = [freezeingPoint, &area](Point3D point) -> bool {
			return freezeingPoint >= area.m_hasTemperature.get(area, point);
		};
		auto [toFreeze, toNotFreeze] = cuboidSetHelper::queryReturnTrueAndFalse(cuboidSet, cuboidCondition, pointCondition);
		if(toFreeze.exists())
			area.m_hasPhaseChanges.m_freezing.getOrCreate(fluid).maybeAdd(toFreeze);
		if(toNotFreeze.exists())
		{
			auto found = area.m_hasPhaseChanges.m_freezing.find(fluid);
			if(found != area.m_hasPhaseChanges.m_freezing.end())
				found->second.maybeRemove(toNotFreeze);
		}
	}
}