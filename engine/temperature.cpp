#include "temperature.h"
#include "actors/actors.h"
#include "area/area.h"
#include "definitions/animalSpecies.h"
#include "objectives/getToSafeTemperature.h"
UnsafeTemperatureEvent::UnsafeTemperatureEvent(Area& area, const ActorIndex a, const Step start) :
	ScheduledEvent(area.m_simulation, AnimalSpecies::getStepsTillDieInUnsafeTemperature(area.getActors().getSpecies(a)), start),
	m_needsSafeTemperature(*area.getActors().m_needsSafeTemperature[a].get()) { }
void UnsafeTemperatureEvent::execute(Simulation&, Area* area) { m_needsSafeTemperature.dieFromTemperature(*area); }
void UnsafeTemperatureEvent::clearReferences(Simulation&, Area*) { m_needsSafeTemperature.m_event.clearPointer(); }
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(Area& area, const ActorIndex a) :
	m_event(area.m_eventSchedule)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
ActorNeedsSafeTemperature::ActorNeedsSafeTemperature(const Json& data, const ActorIndex actor, Area& area) :
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

void ActorNeedsSafeTemperature::setTemperature(Area& area, Temperature temperature)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	AnimalSpeciesId species = actors.getSpecies(actor);
	bool safe = temperature >= AnimalSpecies::getMinimumSafeTemperature(species) && temperature <= AnimalSpecies::getMaximumSafeTemperature(species);
	if(!safe)
	{
		if(!actors.objective_hasNeed(actor, NeedType::temperature))
		{
			std::unique_ptr<Objective> objective = std::make_unique<GetToSafeTemperatureObjective>();
			actors.objective_addNeed(actor, std::move(objective));
		}
		if(!m_event.exists())
			m_event.schedule(area, actor);
	}
	else
		m_event.maybeUnschedule();
}
bool ActorNeedsSafeTemperature::isSafe(Area& area, const Temperature temperature) const
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
	Point3D location = actors.getLocation(actor);
	if(location.empty())
		return true;
	return isSafe(area, area.m_hasTemperature.get(area, location));
}
bool ActorNeedsSafeTemperature::eventExists() { return m_event.exists(); }
Percent ActorNeedsSafeTemperature::dieFromTemperaturePercent(Area& area) const { return isSafeAtCurrentLocation(area) ? Percent::create(0) : m_event.percentComplete(); }