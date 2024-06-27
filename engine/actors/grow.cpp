#include "grow.h"
#include "simulation.h"
#include "util.h"
#include "area.h"
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
	Step delay = std::round(area.m_actors.getSpecies(m_actor).stepsTillFullyGrown / 100.f);
	m_event.resume(delay, *this);
}
void CanGrow::update(Area& area)
{
	Actors& actors = area.getActors();
	const AnimalSpecies& species = actors.getSpecies(m_actor);
	actors.grow_updatePercent(m_actor, m_percentGrown);
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
AnimalGrowthEvent::AnimalGrowthEvent(Area& area, Step delay, CanGrow& cg, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_canGrow(cg) { }
