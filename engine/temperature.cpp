#include "temperature.h"
#include "area/area.h"
#include "datetime.h"
#include "space/nthAdjacentOffsets.h"
#include "config/config.h"
#include "objective.h"
#include "objectives/getToSafeTemperature.h"
#include "simulation/simulation.h"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"
#include "plants.h"
#include "actors/actors.h"
#include "items/items.h"
#include "space/space.h"
#include "definitions/animalSpecies.h"
#include <cmath>
enum class TemperatureZone { Surface, Underground, LavaSea };
TemperatureDelta TemperatureSource::getTemperatureDeltaForRange(const Distance& range)
{
	if(range == 0)
		return m_temperature;
	TemperatureDelta output = m_temperature / std::pow(range.get(), Config::heatDisipatesAtDistanceExponent);
	if(output.absoluteValue() <= Config::heatRadianceMinimum)
		return TemperatureDelta::create(0);
	return output; // subtract radiance mininum from output to smooth transition at edge of affected zone?
}
void TemperatureSource::apply(Area& area)
{
	Space& space = area.getSpace();
	area.m_hasTemperature.addDelta(m_point, m_temperature);
	Distance range = Distance::create(1);
	while(range <= Config::heatDistanceMaximum)
	{
		TemperatureDelta delta = getTemperatureDeltaForRange(range);
		if(delta == 0)
			break;
		for(const Point3D& point : space.getNthAdjacent(m_point, range))
			area.m_hasTemperature.addDelta(point, delta);
		++range;
	}
}
void TemperatureSource::unapply(Area& area)
{
	Space& space = area.getSpace();
	area.m_hasTemperature.addDelta(m_point, m_temperature * -1);
	Distance range = Distance::create(1);
	while(true)
	{
		TemperatureDelta delta = getTemperatureDeltaForRange(range);
		if(delta == 0)
			break;
		for(const Point3D& point : space.getNthAdjacent(m_point, range))
			area.m_hasTemperature.addDelta(point, delta * -1);
		++range;
	}
}
void TemperatureSource::setTemperature(Area& area, const TemperatureDelta& t)
{
	unapply(area);
	m_temperature = t;
	apply(area);
}
void AreaHasTemperature::addTemperatureSource(const Point3D& point, const TemperatureDelta& temperature)
{
	m_sources.emplace(point, m_area, temperature, point);
}
void AreaHasTemperature::removeTemperatureSource(TemperatureSource& temperatureSource)
{
	assert(m_sources.contains(temperatureSource.m_point));
	//TODO: Optimize with iterator.
	m_sources[temperatureSource.m_point].unapply(m_area);
	m_sources.erase(temperatureSource.m_point);
}
TemperatureSource& AreaHasTemperature::getTemperatureSourceAt(const Point3D& point)
{
	return m_sources[point];
}
void AreaHasTemperature::addDelta(const Point3D& point, const TemperatureDelta& delta)
{
	auto found = m_pointDeltaDeltas.find(point);
	if(found == m_pointDeltaDeltas.end())
		m_pointDeltaDeltas.insert(point, const_cast<TemperatureDelta&>(delta));
	else
		found->second += delta;
}
// TODO: optimize by splitting point deltas into different structures for different temperature zones, to avoid having to rerun getAmbientTemperature?
void AreaHasTemperature::applyDeltas()
{
	SmallMap<Point3D, TemperatureDelta> oldDeltaDeltas;
	oldDeltaDeltas.swap(m_pointDeltaDeltas);
	for(auto& [point, deltaDelta] : oldDeltaDeltas)
		m_area.getSpace().temperature_updateDelta(point, deltaDelta);
}
void AreaHasTemperature::setAmbientSurfaceTemperature(const Temperature& temperature)
{
	m_ambiantSurfaceTemperature = temperature;
	Space& space = m_area.getSpace();
	m_area.getPlants().onChangeAmbiantSurfaceTemperature();
	m_area.getActors().onChangeAmbiantSurfaceTemperature();
	m_area.getItems().onChangeAmbiantSurfaceTemperature();
	m_area.m_exteriorPortals.onChangeAmbiantSurfaceTemperature(space, temperature);
	for(auto& [meltingPoint, toMeltOnSurface] : m_aboveGroundPointsByMeltingPoint)
		if(meltingPoint <= temperature)
		{
			CuboidSet toMelt;
			for(const Point3D& point : toMeltOnSurface)
			{
				// This point has been processed already as part of another point's group.
				if(toMelt.contains(point))
					continue;
				const MaterialTypeId& materialType = space.solid_get(point);
				for(const Cuboid& cuboid : space.collectAdjacentsWithCondition(point, [&](const Point3D& b){ return space.solid_get(b) == materialType; }))
					toMelt.maybeAdd(cuboid);
			}
			for(const Cuboid& cuboid : toMelt)
				for(const Point3D& point : cuboid)
					space.temperature_melt(point);
			m_aboveGroundPointsByMeltingPoint[meltingPoint].clear();
		}
		else
			break;
	SmallMap<FluidTypeId, SmallSet<Point3D>> toFreeze;
	for(auto& [meltingPoint, fluidGroups] : m_aboveGroundFluidGroupsByMeltingPoint)
	{
		const FluidTypeId& fluidType = fluidGroups.front()->m_fluidType;
		// TODO: replace getOrCreate with create.
		auto& pointSet = toFreeze.getOrCreate(fluidType);
		if(meltingPoint > temperature)
		{
			for(FluidGroup* fluidGroup : fluidGroups)
			{
				for(const Cuboid& cuboid : fluidGroup->m_drainQueue.m_set)
					for(const Point3D point : cuboid)
						if(space.isExposedToSky(point))
							pointSet.insert(point);
				fluidGroup->m_aboveGround = false;
			}
			for(const auto& [fluidTypeToFreeze, pointsToFreeze] : toFreeze)
				for(const Point3D& point : pointsToFreeze)
					space.temperature_freeze(point, fluidTypeToFreeze);
			// Any possible freezing at this temperature has happened so clear groupsByMeltingPoint for this temperature.
			// Don't delete the empty set, it will be reused eventually.
			fluidGroups.clear();
		}
		else
			break;
	}
}
void AreaHasTemperature::updateAmbientSurfaceTemperature()
{
	// TODO: Latitude and altitude.
	Temperature dailyAverage = getDailyAverageAmbientSurfaceTemperature();
	static Temperature maxDailySwing = Temperature::create(35);
	static int8_t hottestHourOfDay = 14;
	int8_t hour = DateTime(m_area.m_simulation.m_step).hour;
	int8_t hoursFromHottestHourOfDay = std::abs((int32_t)hottestHourOfDay - hour);
	int8_t halfDay = Config::hoursPerDay / 2;
	setAmbientSurfaceTemperature(dailyAverage + ((maxDailySwing * (std::max(0, halfDay - hoursFromHottestHourOfDay))) / halfDay) - (maxDailySwing / 2));
}
void AreaHasTemperature::maybeAddMeltableSolidPointAboveGround(const Point3D& point)
{
	Space& space = m_area.getSpace();
	assert(space.isExposedToSky(point));
	assert(space.solid_isAny(point));
	m_aboveGroundPointsByMeltingPoint[MaterialType::getMeltingPoint(space.solid_get(point))].maybeInsert(point);
}
// Must be run before point is set no longer solid if above ground.
void AreaHasTemperature::maybeRemoveMeltableSolidPointAboveGround(const Point3D& point)
{
	Space& space = m_area.getSpace();
	assert(space.solid_isAny(point));
	m_aboveGroundPointsByMeltingPoint.at(MaterialType::getMeltingPoint(space.solid_get(point))).maybeErase(point);
}
void AreaHasTemperature::addFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup)
{
	const Temperature& freezing = FluidType::getFreezingPoint(fluidGroup.m_fluidType);
	m_aboveGroundFluidGroupsByMeltingPoint[freezing].insert(&fluidGroup);
}
void AreaHasTemperature::maybeRemoveFreezeableFluidGroupAboveGround(FluidGroup& fluidGroup)
{
	const Temperature& freezing = FluidType::getFreezingPoint(fluidGroup.m_fluidType);
	assert(m_aboveGroundFluidGroupsByMeltingPoint.contains(freezing));
	auto& groups = m_aboveGroundFluidGroupsByMeltingPoint[freezing];
	m_aboveGroundFluidGroupsByMeltingPoint[freezing].maybeErase(&fluidGroup);
	if(groups.empty())
		m_aboveGroundFluidGroupsByMeltingPoint.erase(freezing);
}
Temperature AreaHasTemperature::getDailyAverageAmbientSurfaceTemperature() const
{
	// TODO: Latitude and altitude.
	static Temperature yearlyHottestDailyAverage = Temperature::create(290);
	static Temperature yearlyColdestDailyAverage = Temperature::create(270);
	static int16_t dayOfYearOfSolstice = Config::daysPerYear / 2;
	int16_t day = DateTime(m_area.m_simulation.m_step).day;
	int16_t daysFromSolstice = std::abs(day - (int32_t)dayOfYearOfSolstice);
	return yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
}
UnsafeTemperatureEvent::UnsafeTemperatureEvent(Area& area, const ActorIndex& a, const Step start) :
	ScheduledEvent(area.m_simulation, AnimalSpecies::getStepsTillDieInUnsafeTemperature(area.getActors().getSpecies(a)), start),
	m_needsSafeTemperature(*area.getActors().m_needsSafeTemperature[a].get()) { }
void UnsafeTemperatureEvent::execute(Simulation&, Area* area) { m_needsSafeTemperature.dieFromTemperature(*area); }
void UnsafeTemperatureEvent::clearReferences(Simulation&, Area*) { m_needsSafeTemperature.m_event.clearPointer(); }
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(Area& area, const ActorIndex& a) :
	m_event(area.m_eventSchedule)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(const Json& data, const ActorIndex& actor, Area& area) :
	m_event(area.m_eventSchedule)
{
	m_actor.setIndex(actor, area.getActors().m_referenceData);
	if(data.contains("eventStart"))
		m_event.schedule(area, actor, data["eventStart"].get<Step>());
}
Json ActorNeedsSafeTemperature::toJson() const
{
	Json data;
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
void ActorNeedsSafeTemperature::onChange(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	actors.grow_updateGrowingStatus(actor);
	if(!isSafeAtCurrentLocation(area))
	{
		if(!actors.objective_hasNeed(actor, NeedType::temperature))
		{
			std::unique_ptr<Objective> objective = std::make_unique<GetToSafeTemperatureObjective>();
			actors.objective_addNeed(actor, std::move(objective));
		}
		if(!m_event.exists())
			m_event.schedule(area, actor);
	}
	else if(m_event.exists())
		m_event.unschedule();
}
void ActorNeedsSafeTemperature::dieFromTemperature(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	actors.die(actor, CauseOfDeath::temperature);
}
void ActorNeedsSafeTemperature::unschedule()
{
	m_event.unschedule();
}
bool ActorNeedsSafeTemperature::isSafe(Area& area, const Temperature& temperature) const
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	AnimalSpeciesId species = actors.getSpecies(actor);
	return temperature >= AnimalSpecies::getMinimumSafeTemperature(species) && temperature <= AnimalSpecies::getMaximumSafeTemperature(species);
}
bool ActorNeedsSafeTemperature::isSafeAtCurrentLocation(Area& area) const
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	if(actors.getLocation(actor).empty())
		return true;
	return isSafe(area, area.getSpace().temperature_get(actors.getLocation(actor)));
}