#include "grow.h"
#include "actors.h"
#include "../simulation.h"
#include "../util.h"
#include "../area.h"

void Actors::grow_maybeStart(ActorIndex index)
{
	m_canGrow.at(index)->maybeStart(m_area);
}
void Actors::grow_stop(ActorIndex index)
{
	m_canGrow.at(index)->stop();
}
void Actors::grow_updateGrowingStatus(ActorIndex index)
{
	m_canGrow.at(index)->updateGrowingStatus(m_area);
}
void Actors::grow_setPercent(ActorIndex index, Percent percentGrown)
{
	m_canGrow.at(index)->setGrowthPercent(m_area, percentGrown);
}
bool Actors::grow_isGrowing(ActorIndex index) const
{
	return m_canGrow.at(index)->isGrowing();
}
Percent Actors::grow_getPercent(ActorIndex index) const
{
	return m_canGrow.at(index)->growthPercent();
}
bool Actors::grow_getEventExists(ActorIndex index) const
{
	return m_canGrow.at(index)->getEvent().exists();
}
Percent Actors::grow_getEventPercent(ActorIndex index) const
{
	return m_canGrow.at(index)->getEvent().percentComplete();
}
Step Actors::grow_getEventStep(ActorIndex index) const
{
	return m_canGrow.at(index)->getEvent().getStep();
}
bool Actors::grow_eventIsPaused(ActorIndex index) const
{
	return m_canGrow.at(index)->getEvent().isPaused();
}
CanGrow::CanGrow(Area& area, ActorIndex a, Percent pg) :
	m_event(area.m_eventSchedule), m_actor(a), m_percentGrown(pg) { }
/*
CanGrow::CanGrow(const Json& data, ActorIndex a, Simulation& s) : m_event(s.m_eventSchedule), m_actor(a), m_percentGrown(data["percentGrown"].get<Percent>())
{
	if(data.contains("eventStart"))
		m_event.schedule(data["eventDuration"].get<Step>(), *this, data["eventStart"].get<Step>());
}
Json CanGrow::toJson() const
{
	Json data({{"percentGrown", growthPercent()}});
	if(m_event.exists())
	{
		data["eventStart"] = m_event.getStartStep();
		data["eventDuration"] = m_event.duration();
	}
	return data;
}
*/
void CanGrow::setGrowthPercent(Area& area, Percent percent)
{
	m_percentGrown = percent;
	// Restart the growth event.
	stop();
	m_event.reset();
	update(area);
	updateGrowingStatus(area);
}
void CanGrow::updateGrowingStatus(Area& area)
{
	if(canGrowCurrently(area))
	{
		if(m_event.isPaused())
			scheduleEvent(area);
	}
	else if(!m_event.isPaused())
		m_event.pause();
}
bool CanGrow::canGrowCurrently(Area& area) const
{
	Actors& actors = area.getActors();
	return m_percentGrown != 100 && actors.getAge(m_actor) < actors.getSpecies(m_actor).stepsTillFullyGrown && 
		!actors.eat_isHungry(m_actor) && !actors.drink_isThirsty(m_actor) && 
		actors.temperature_isSafeAtCurrentLocation(m_actor);
}
Percent CanGrow::growthPercent() const
{
	return m_percentGrown;
}
void CanGrow::scheduleEvent(Area& area)
{
	Step delay = std::round(area.getActors().getSpecies(m_actor).stepsTillFullyGrown / 100.f);
	m_event.resume(delay, area, *this);
}
void CanGrow::update(Area& area)
{
	Actors& actors = area.getActors();
	const AnimalSpecies& species = actors.getSpecies(m_actor);
	actors.grow_setPercent(m_actor, m_percentGrown);
	const Shape& shape = species.shapeForPercentGrown(m_percentGrown);
	if(shape != actors.getShape(m_actor))
		actors.setShape(m_actor, shape);
	if(m_percentGrown != 100 && species.stepsTillFullyGrown > actors.getAge(m_actor) && m_event.isPaused())
		scheduleEvent(area);
}
void CanGrow::stop()
{
	if(!m_event.isPaused())
		m_event.pause();
}
void CanGrow::maybeStart(Area& area)
{
	if(m_event.isPaused())
		updateGrowingStatus(area);
}
void CanGrow::increment(Area& area)
{
	++m_percentGrown;
	m_event.reset();
	update(area);
}
AnimalGrowthEvent::AnimalGrowthEvent(Step delay, Area& area, CanGrow& cg, Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_canGrow(cg) { }
