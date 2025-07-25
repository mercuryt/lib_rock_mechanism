#include "sleep.h"
#include "actors/actors.h"
#include "definitions/animalSpecies.h"
#include "area/area.h"
#include "designations.h"
#include "simulation/simulation.h"
#include "numericTypes/types.h"
#include "plants.h"
#include "objectives/sleep.h"
#include <cassert>
// Sleep Event.
SleepEvent::SleepEvent(Simulation& simulation, const Step& step, MustSleep& ns, const Step start) :
	ScheduledEvent(simulation, step, start), m_needsSleep(ns) { }
void SleepEvent::execute(Simulation&, Area* area){ m_needsSleep.wakeUp(*area); }
void SleepEvent::clearReferences(Simulation&, Area*){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(Simulation& simulation, const Step& step, MustSleep& ns, const Step start) :
	ScheduledEvent(simulation, step, start), m_needsSleep(ns) { }
void TiredEvent::execute(Simulation&, Area* area){ m_needsSleep.tired(*area); }
void TiredEvent::clearReferences(Simulation&, Area*){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Needs Sleep.
MustSleep::MustSleep(Area& area, const ActorIndex& a) :
	m_sleepEvent(area.m_eventSchedule), m_tiredEvent(area.m_eventSchedule)
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
}
void MustSleep::scheduleTiredEvent(Area& area)
{
	Actors& actors = area.getActors();
	Step frequency = AnimalSpecies::getStepsSleepFrequency(area.getActors().getSpecies(m_actor.getIndex(actors.m_referenceData)));
	m_tiredEvent.schedule(area.m_simulation, frequency, *this);
}
MustSleep::MustSleep(Area& area, const Json& data, const ActorIndex& a) :
	m_sleepEvent(area.m_eventSchedule), m_tiredEvent(area.m_eventSchedule),
	m_location(data.contains("location") ? data["location"].get<Point3D>() : Point3D::null()),
	m_needsSleep(data["needsSleep"].get<bool>()), m_isAwake(data["isAwake"].get<bool>())
{
	m_actor.setIndex(a, area.getActors().m_referenceData);
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
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
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
		Actors& actors = area.getActors();
		Step overide = AnimalSpecies::getStepsTillSleepOveride(area.getActors().getSpecies(m_actor.getIndex(actors.m_referenceData)));
		m_tiredEvent.schedule(area.m_simulation, overide, *this);
		if(!area.getActors().objective_hasNeed(m_actor.getIndex(actors.m_referenceData), NeedType::sleep))
			makeSleepObjective(area);
	}
}
// Voluntary sleep.
void MustSleep::sleep(Area& area)
{
	Actors& actors = area.getActors();
	Step duration = AnimalSpecies::getStepsSleepDuration(area.getActors().getSpecies(m_actor.getIndex(actors.m_referenceData)));
	sleep(area, duration);
}
// Involuntary sleep.
void MustSleep::passout(Area& area, const Step& duration) { sleep(area, duration, true); }
void MustSleep::sleep(Area& area, const Step& duration, bool force)
{
	assert(m_isAwake);
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	actors.move_clearAllEventsAndTasks(actor);
	m_isAwake = false;
	m_force = force;
	m_tiredEvent.maybeUnschedule();
	m_sleepEvent.schedule(area.m_simulation, duration, *this);
	if(m_objective != nullptr)
		actors.move_pathRequestMaybeCancel(actor);
	actors.vision_clearCanSee(actor);
}
void MustSleep::wakeUp(Area& area)
{
	assert(!m_isAwake);
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
	m_isAwake = true;
	m_needsSleep = false;
	Step frequency = AnimalSpecies::getStepsSleepFrequency(area.getActors().getSpecies(actor));
	m_tiredEvent.schedule(area.m_simulation, frequency, *this);
	actors.stamina_setFull(actor);
	// Objective complete releases all reservations.
	if(m_objective != nullptr)
	{
		actors.objective_complete(actor, *m_objective);
		m_objective = nullptr;
	}
	actors.vision_createRequestIfCanSee(actor);
}
void MustSleep::makeSleepObjective(Area& area)
{
	assert(m_isAwake);
	assert(m_objective == nullptr);
	Actors& actors = area.getActors();
	ActorIndex actor = m_actor.getIndex(actors.m_referenceData);
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
	Actors& actors = area.getActors();
	ActorIndex index = m_actor.getIndex(actors.m_referenceData);
	Step overide = AnimalSpecies::getStepsTillSleepOveride(area.getActors().getSpecies(index));
	m_tiredEvent.schedule(area.m_simulation, overide, *this);
	actors.vision_createRequestIfCanSee(index);
	//TODO: partial stamina recovery.
}
void MustSleep::setLocation(const Point3D& point)
{
	m_location = point;
}
void MustSleep::unschedule()
{
	m_tiredEvent.maybeUnschedule();
}
