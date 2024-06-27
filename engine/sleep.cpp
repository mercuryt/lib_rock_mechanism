#include "sleep.h"
#include "area.h"
#include "designations.h"
#include "hasShape.h"
#include "objective.h"
#include "pathRequest.h"
#include "simulation.h"
#include "terrainFacade.h"
#include "types.h"
#include <cassert>
// Sleep Event.
SleepEvent::SleepEvent(Simulation& simulation, Step step, MustSleep& ns, bool f, const Step start) :
	ScheduledEvent(simulation, step, start), m_needsSleep(ns), m_force(f) { }
void SleepEvent::execute(Simulation&, Area* area){ m_needsSleep.wakeUp(*area); }
void SleepEvent::clearReferences(Simulation&, Area*){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(Simulation& simulation, Step step, MustSleep& ns, const Step start) :
	ScheduledEvent(simulation, step, start), m_needsSleep(ns) { }
void TiredEvent::execute(Simulation&, Area* area){ m_needsSleep.tired(*area); }
void TiredEvent::clearReferences(Simulation&, Area*){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Needs Sleep.
MustSleep::MustSleep(Area& area, ActorIndex a) :
	m_sleepEvent(area.m_eventSchedule), m_tiredEvent(area.m_eventSchedule), m_actor(a) { }
void MustSleep::scheduleTiredEvent(Area& area)
{
	m_tiredEvent.schedule(area.m_actors.getSpecies(m_actor).stepsSleepFrequency, *this);
}
/*
MustSleep::MustSleep(const Json data, ActorIndex a, Simulation& s, const AnimalSpecies& species) :
	m_sleepEvent(s.m_eventSchedule), m_tiredEvent(s.m_eventSchedule),
	m_actor(a), m_location(data.contains("location") ? data["location"].get<BlockIndex>() : BLOCK_INDEX_MAX),
	m_needsSleep(data["needsSleep"].get<bool>()), m_isAwake(data["isAwake"].get<bool>())
{
	if(data.contains("sleepEventStart"))
		m_sleepEvent.schedule(species.stepsSleepDuration, *this, data["sleepEventStart"].get<Step>());
	if(data.contains("tiredEventStart"))
		m_tiredEvent.schedule(species.stepsSleepFrequency, *this, data["tiredEventStart"].get<Step>());
}
Json MustSleep::toJson() const
{
	Json data;
	if(m_location != BLOCK_INDEX_MAX)
		data["location"] = m_location;
	data["needsSleep"] = m_needsSleep;
	data["isAwake"] = m_isAwake;
	if(m_sleepEvent.exists())
		data["sleepEventStart"] = m_sleepEvent.getStartStep();
	if(m_tiredEvent.exists())
		data["tiredEventStart"] = m_tiredEvent.getStartStep();
	return data;
}
*/
void MustSleep::notTired(Area& area)
{
	Actors& actors = area.getActors();
	assert(m_isAwake);
	if(m_objective != nullptr)
		actors.objective_complete(m_actor, *m_objective);
	m_needsSleep = false;
	m_tiredEvent.unschedule();
	m_tiredEvent.schedule(actors.getSpecies(m_actor).stepsSleepFrequency, *this);
}
void MustSleep::tired(Area& area)
{
	assert(m_isAwake);
	if(m_needsSleep)
		sleep(area);
	else
	{
		m_needsSleep = true;
		m_tiredEvent.maybeUnschedule();
		Actors& actors = area.getActors();
		m_tiredEvent.schedule(actors.getSpecies(m_actor)j.stepsTillSleepOveride, *this);
		makeSleepObjective(area);
	}
}
// Voluntary sleep.
void MustSleep::sleep(Area& area) { sleep(area, area.getActors().getSpecies(m_actor).stepsSleepDuration); }
// Involuntary sleep.
void MustSleep::passout(Area& area, Step duration) { sleep(area, duration, true); }
void MustSleep::sleep(Area& area, Step duration, bool force)
{
	Actors& actors = area.getActors();
	assert(m_isAwake);
	actors.move_clearAllEventsAndTasks(m_actor);
	m_isAwake = false;
	m_tiredEvent.maybeUnschedule();
	m_sleepEvent.schedule(duration, *this, force);
	if(m_objective != nullptr)
		actors.move_pathRequestMaybeCancel(m_actor);
	actors.vision_clearFacade(m_actor);
}
void MustSleep::wakeUp(Area& area)
{
	assert(!m_isAwake);
	Actors& actors = area.getActors();
	m_isAwake = true;
	m_needsSleep = false;
	m_tiredEvent.schedule(actors.getSpecies(m_actor).stepsSleepFrequency, *this);
	actors.stamina_setFull(m_actor);
	// Objective complete releases all reservations.
	if(m_objective != nullptr)
		actors.objective_complete(m_actor, *m_objective);
	actors.vision_createFacadeIfCanSee(m_actor);
}
void MustSleep::makeSleepObjective(Area& area)
{
	assert(m_isAwake);
	assert(m_objective == nullptr);
	Actors& actors = area.getActors();
	std::unique_ptr<Objective> objective = std::make_unique<SleepObjective>(m_actor);
	m_objective = static_cast<SleepObjective*>(objective.get());
	actors.objective_addNeed(std::move(objective));
}
void MustSleep::wakeUpEarly(Area& area)
{
	assert(!m_isAwake);
	assert(m_needsSleep == true);
	m_isAwake = true;
	m_sleepEvent.pause();
	m_tiredEvent.schedule(area.getActors().getSpecies(m_actor).stepsTillSleepOveride, *this);
	//TODO: partial stamina recovery.
}
void MustSleep::setLocation(BlockIndex block)
{
	m_location = block;
}
void MustSleep::onDeath()
{
	m_tiredEvent.maybeUnschedule();
}
