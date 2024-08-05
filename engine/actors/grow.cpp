#include "grow.h"
#include "actors.h"
#include "../simulation.h"
#include "../util.h"
#include "../area.h"
#include "../animalSpecies.h"
#include "types.h"

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
	m_event(area.m_eventSchedule), m_percentGrown(pg)
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
}
CanGrow::CanGrow(Area& area, const Json& data, ActorIndex a) : m_event(area.m_eventSchedule), m_percentGrown(data["percentGrown"].get<Percent>())
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
	if(data.contains("eventStart"))
		m_event.schedule(data["eventDuration"].get<Step>(), area, *this, data["eventStart"].get<Step>());
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
	ActorIndex actor = m_actor.getIndex();
	return m_percentGrown != 100 && actors.getAge(actor) < actors.getSpecies(actor).stepsTillFullyGrown && 
		!actors.eat_isHungry(actor) && !actors.drink_isThirsty(actor) && 
		actors.temperature_isSafeAtCurrentLocation(actor);
}
Percent CanGrow::growthPercent() const
{
	return m_percentGrown;
}
void CanGrow::scheduleEvent(Area& area)
{
	ActorIndex actor = m_actor.getIndex();
	Step delay = Step::create(std::round(area.getActors().getSpecies(actor).stepsTillFullyGrown.get() / 100.f));
	m_event.resume(delay, area, *this);
}
void CanGrow::update(Area& area)
{
	ActorIndex actor = m_actor.getIndex();
	Actors& actors = area.getActors();
	const AnimalSpecies& species = actors.getSpecies(actor);
	actors.grow_setPercent(actor, m_percentGrown);
	const Shape& shape = species.shapeForPercentGrown(m_percentGrown);
	if(shape != actors.getShape(actor))
		actors.setShape(actor, shape);
	if(m_percentGrown != 100 && species.stepsTillFullyGrown > actors.getAge(actor) && m_event.isPaused())
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
