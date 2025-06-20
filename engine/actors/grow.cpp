#include "grow.h"
#include "actors.h"
#include "../simulation/simulation.h"
#include "../util.h"
#include "../area/area.h"
#include "../definitions/animalSpecies.h"
#include "../numericTypes/types.h"
#include "../hasShapes.hpp"

void Actors::grow_maybeStart(const ActorIndex& index)
{
	m_canGrow[index]->maybeStart(m_area);
}
void Actors::grow_stop(const ActorIndex& index)
{
	m_canGrow[index]->stop();
}
void Actors::grow_updateGrowingStatus(const ActorIndex& index)
{
	m_canGrow[index]->updateGrowingStatus(m_area);
}
void Actors::grow_setPercent(const ActorIndex& index, const Percent& percentGrown)
{
	m_canGrow[index]->setGrowthPercent(m_area, percentGrown);
}
bool Actors::grow_isGrowing(const ActorIndex& index) const
{
	return m_canGrow[index]->isGrowing();
}
Percent Actors::grow_getPercent(const ActorIndex& index) const
{
	return m_canGrow[index]->growthPercent();
}
bool Actors::grow_getEventExists(const ActorIndex& index) const
{
	return m_canGrow[index]->getEvent().exists();
}
Percent Actors::grow_getEventPercent(const ActorIndex& index) const
{
	return m_canGrow[index]->getEvent().percentComplete();
}
Step Actors::grow_getEventStep(const ActorIndex& index) const
{
	return m_canGrow[index]->getEvent().getStep();
}
bool Actors::grow_eventIsPaused(const ActorIndex& index) const
{
	return m_canGrow[index]->getEvent().isPaused();
}
CanGrow::CanGrow(Area& area, const ActorIndex& a, const Percent& pg) :
	m_event(area.m_eventSchedule), m_percentGrown(pg)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
CanGrow::CanGrow(Area& area, const Json& data, const ActorIndex& a) : m_event(area.m_eventSchedule), m_percentGrown(data["percentGrown"].get<Percent>())
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
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
void CanGrow::setGrowthPercent(Area& area, const Percent& percent)
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
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	return m_percentGrown != 100 && actors.getAge(actor) < AnimalSpecies::getStepsTillFullyGrown(actors.getSpecies(actor)) &&
		!actors.eat_isHungry(actor) && !actors.drink_isThirsty(actor) &&
		actors.temperature_isSafeAtCurrentLocation(actor);
}
Percent CanGrow::growthPercent() const
{
	return m_percentGrown;
}
void CanGrow::scheduleEvent(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	Step delay = Step::create(std::round((float)AnimalSpecies::getStepsTillFullyGrown(area.getActors().getSpecies(actor)).get() / 100.f));
	m_event.resume(delay, area, *this);
}
void CanGrow::update(Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	AnimalSpeciesId species = actors.getSpecies(actor);
	ShapeId shape = AnimalSpecies::shapeForPercentGrown(species, m_percentGrown);
	if(shape != actors.getShape(actor))
		actors.setShape(actor, shape);
	if(m_percentGrown != 100 && AnimalSpecies::getStepsTillFullyGrown(species) > actors.getAge(actor) && m_event.isPaused())
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
void CanGrow::unschedule()
{
	m_event.unschedule();
}
AnimalGrowthEvent::AnimalGrowthEvent(const Step& delay, Area& area, CanGrow& cg, const Step start) :
	ScheduledEvent(area.m_simulation, delay, start), m_canGrow(cg) { }
