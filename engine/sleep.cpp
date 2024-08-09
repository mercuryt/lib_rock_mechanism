#include "sleep.h"
#include "actors/actors.h"
#include "animalSpecies.h"
#include "area.h"
#include "designations.h"
#include "simulation.h"
#include "types.h"
#include "plants.h"
#include "objectives/sleep.h"
#include <cassert>
// Sleep Event.
SleepEvent::SleepEvent(Simulation& simulation, Step step, MustSleep& ns, const Step start) :
	ScheduledEvent(simulation, step, start), m_needsSleep(ns) { }
void SleepEvent::execute(Simulation&, Area* area){ m_needsSleep.wakeUp(*area); }
void SleepEvent::clearReferences(Simulation&, Area*){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(Simulation& simulation, Step step, MustSleep& ns, const Step start) :
	ScheduledEvent(simulation, step, start), m_needsSleep(ns) { }
void TiredEvent::execute(Simulation&, Area* area){ m_needsSleep.tired(*area); }
void TiredEvent::clearReferences(Simulation&, Area*){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Needs Sleep.
MustSleep::MustSleep(Area& area, ActorIndex a) :
	m_sleepEvent(area.m_eventSchedule), m_tiredEvent(area.m_eventSchedule)
{ 
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
}
void MustSleep::scheduleTiredEvent(Area& area)
{
	Step frequency = AnimalSpecies::getStepsSleepFrequency(area.getActors().getSpecies(m_actor.getIndex()));
	m_tiredEvent.schedule(area.m_simulation, frequency, *this);
}
MustSleep::MustSleep(Area& area, const Json& data, ActorIndex a) :
	m_sleepEvent(area.m_eventSchedule), m_tiredEvent(area.m_eventSchedule),
	m_location(data.contains("location") ? data["location"].get<BlockIndex>() : BlockIndex::null()),
	m_needsSleep(data["needsSleep"].get<bool>()), m_isAwake(data["isAwake"].get<bool>())
{
	m_actor.setTarget(area.getActors().getReferenceTarget(a));
	if(data.contains("sleepEventStart"))
		m_sleepEvent.schedule(area.m_simulation, data["sleepEventDuration"].get<Step>(), *this, data["sleepEventStart"].get<Step>());
	if(data.contains("tiredEventStart"))
		m_tiredEvent.schedule(area.m_simulation, data["tiredEventDuration"].get<Step>(), *this, data["tiredEventStart"].get<Step>());
}
Json MustSleep::toJson() const
{
	Json data;
	if(m_location.exists())
		data["location"] = m_location;
	data["needsSleep"] = m_needsSleep;
	data["isAwake"] = m_isAwake;
	if(m_force)
		data["force"] = true;
	if(m_sleepEvent.exists())
	{
		data["sleepEventStart"] = m_sleepEvent.getStartStep();
		data["sleepEventDuration"] = m_sleepEvent.duration();
	}
	if(m_tiredEvent.exists())
	{
		data["tiredEventStart"] = m_tiredEvent.getStartStep();
		data["tiredEventDuration"] = m_tiredEvent.duration();
	}
	return data;
}
void MustSleep::notTired(Area& area)
{
	Actors& actors = area.getActors();
	assert(m_isAwake);
	ActorIndex actor = m_actor.getIndex();
	if(m_objective != nullptr)
		actors.objective_complete(actor, *m_objective);
	m_needsSleep = false;
	m_tiredEvent.unschedule();
	Step frequency = AnimalSpecies::getStepsSleepFrequency(area.getActors().getSpecies(actor));
	m_tiredEvent.schedule(area.m_simulation, frequency, *this);
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
		Step overide = AnimalSpecies::getStepsTillSleepOveride(area.getActors().getSpecies(m_actor.getIndex()));
		m_tiredEvent.schedule(area.m_simulation, overide, *this);
		makeSleepObjective(area);
	}
}
// Voluntary sleep.
void MustSleep::sleep(Area& area)
{
	Step duration = AnimalSpecies::getStepsSleepDuration(area.getActors().getSpecies(m_actor.getIndex()));
	sleep(area, duration);
}
// Involuntary sleep.
void MustSleep::passout(Area& area, Step duration) { sleep(area, duration, true); }
void MustSleep::sleep(Area& area, Step duration, bool force)
{
	assert(m_isAwake);
	ActorIndex actor = m_actor.getIndex();
	Actors& actors = area.getActors();
	actors.move_clearAllEventsAndTasks(actor);
	m_isAwake = false;
	m_force = force;
	m_tiredEvent.maybeUnschedule();
	m_sleepEvent.schedule(area.m_simulation, duration, *this);
	if(m_objective != nullptr)
		actors.move_pathRequestMaybeCancel(actor);
	actors.vision_clearFacade(actor);
}
void MustSleep::wakeUp(Area& area)
{
	assert(!m_isAwake);
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex();
	m_isAwake = true;
	m_needsSleep = false;
	Step frequency = AnimalSpecies::getStepsSleepFrequency(area.getActors().getSpecies(actor));
	m_tiredEvent.schedule(area.m_simulation, frequency, *this);
	actors.stamina_setFull(actor);
	// Objective complete releases all reservations.
	if(m_objective != nullptr)
		actors.objective_complete(actor, *m_objective);
	actors.vision_createFacadeIfCanSee(actor);
}
void MustSleep::makeSleepObjective(Area& area)
{
	assert(m_isAwake);
	assert(m_objective == nullptr);
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex();
	std::unique_ptr<Objective> objective = std::make_unique<SleepObjective>();
	m_objective = static_cast<SleepObjective*>(objective.get());
	actors.objective_addNeed(actor, std::move(objective));
}
void MustSleep::wakeUpEarly(Area& area)
{
	assert(!m_isAwake);
	assert(m_needsSleep == true);
	m_isAwake = true;
	m_sleepEvent.pause();
	Step overide = AnimalSpecies::getStepsTillSleepOveride(area.getActors().getSpecies(m_actor.getIndex()));
	m_tiredEvent.schedule(area.m_simulation, overide, *this);
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
