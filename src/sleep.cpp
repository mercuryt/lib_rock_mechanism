#include "sleep.h"
#include "area.h"
#include "path.h"
#include <cassert>
// Sleep Event.
SleepEvent::SleepEvent(Step step, MustSleep& ns) : ScheduledEventWithPercent(ns.m_actor.getSimulation(), step), m_needsSleep(ns) { }
void SleepEvent::execute(){ m_needsSleep.wakeUp(); }
void SleepEvent::clearReferences(){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(Step step, MustSleep& ns) : ScheduledEventWithPercent(ns.m_actor.getSimulation(), step), m_needsSleep(ns) { }
void TiredEvent::execute(){ m_needsSleep.tired(); }
void TiredEvent::clearReferences(){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Threaded Task.
SleepThreadedTask::SleepThreadedTask(SleepObjective& so) : ThreadedTask(so.m_actor.getThreadedTaskEngine()), m_sleepObjective(so), m_sleepAtCurrentLocation(false) { }
void SleepThreadedTask::readStep()
{
	auto& actor = m_sleepObjective.m_actor;
	assert(m_sleepObjective.m_actor.m_mustSleep.m_location == nullptr);
	const Block* maxDesireCandidate = nullptr;
	const Block* outdoorCandidate = nullptr;
	const Block* indoorCandidate = nullptr;
	std::function<bool(const Block&)> condition = [&](const Block& block)
	{
		uint32_t desire = m_sleepObjective.desireToSleepAt(block);
		if(desire == 3)
		{
			maxDesireCandidate = &block;
			return true;
		}
		else if(indoorCandidate == nullptr && desire == 2)
			indoorCandidate = &block;	
		else if(outdoorCandidate == nullptr && desire == 1)
			outdoorCandidate = &block;	
		return false;
	};
	m_findsPath.pathToPredicate(actor, condition);
	if(m_findsPath.getPath().empty())
	{
		// If the current location is the max desired then set sleep at current to true.
		if(maxDesireCandidate == actor.m_location)
		{
			assert(maxDesireCandidate != nullptr);
			m_sleepAtCurrentLocation = true;
		}
		// No max desire target found, try to at least get indoors
		else if(indoorCandidate != nullptr)
		{
			if(indoorCandidate == actor.m_location)
				m_sleepAtCurrentLocation = true;
			else
				m_findsPath.pathToBlock(actor, *indoorCandidate);
		}
		else if(outdoorCandidate != nullptr)
		{
			// sleep outdoors.
			if(outdoorCandidate == actor.m_location)
				m_sleepAtCurrentLocation = true;
			else
				m_findsPath.pathToBlock(actor, *outdoorCandidate);
		}
	}
}
void SleepThreadedTask::writeStep()
{
	auto& actor = m_sleepObjective.m_actor;
	m_findsPath.cacheMoveCosts(actor);
	if(m_findsPath.getPath().empty())
	{
		if(m_sleepAtCurrentLocation)
			actor.m_mustSleep.sleep();
		else
			actor.m_hasObjectives.cannotFulfillNeed(m_sleepObjective);
	}
	else
		actor.m_canMove.setPath(m_findsPath.getPath());
}
void SleepThreadedTask::clearReferences() { m_sleepObjective.m_threadedTask.clearPointer(); }
// Sleep Objective.
SleepObjective::SleepObjective(Actor& a) : Objective(Config::sleepObjectivePriority), m_actor(a), m_threadedTask(a.getThreadedTaskEngine()) { }
void SleepObjective::execute()
{
	if(m_actor.m_location == m_actor.m_mustSleep.m_location)
	{
		assert(m_actor.m_mustSleep.m_location != nullptr);
		m_actor.m_mustSleep.sleep();
	}
	else if(m_actor.m_mustSleep.m_location == nullptr)
		m_threadedTask.create(*this);
	else
		m_actor.m_canMove.setDestination(*m_actor.m_mustSleep.m_location);
}
uint32_t SleepObjective::desireToSleepAt(const Block& block)
{
	if(block.m_reservable.isFullyReserved(*m_actor.getFaction()) || !m_actor.m_needsSafeTemperature.isSafe(block.m_blockHasTemperature.get()))
		return 0;
	if(block.m_area->m_hasSleepingSpots.containsUnassigned(block))
		return 3;
	if(block.m_outdoors)
		return 1;
	else
		return 2;
}
SleepObjective::~SleepObjective() { m_actor.m_mustSleep.m_objective = nullptr; }
// Needs Sleep.
MustSleep::MustSleep(Actor& a) : m_actor(a), m_location(nullptr), m_sleepEvent(a.getEventSchedule()), m_tiredEvent(a.getEventSchedule()), m_objective(nullptr), m_needsSleep(false), m_isAwake(true)
{
	m_tiredEvent.schedule(m_actor.m_species.stepsSleepFrequency, *this);
}
void MustSleep::tired()
{
	assert(m_isAwake);
	if(m_needsSleep)
		sleep();
	else
	{
		m_needsSleep = true;
		m_tiredEvent.schedule(m_actor.m_species.stepsTillSleepOveride, *this);
		makeSleepObjective();
	}
}
void MustSleep::sleep()
{
	assert(m_isAwake);
	m_isAwake = false;
	m_tiredEvent.maybeUnschedule();
	m_sleepEvent.schedule(m_actor.m_species.stepsSleepDuration, *this);
}
void MustSleep::wakeUp()
{
	assert(!m_isAwake);
	m_isAwake = true;
	m_tiredEvent.schedule(m_actor.m_species.stepsSleepFrequency, *this);
	m_actor.m_stamina.setFull();
}
void MustSleep::makeSleepObjective()
{
	assert(m_isAwake);
	assert(m_objective == nullptr);
	std::unique_ptr<Objective> objective = std::make_unique<SleepObjective>(m_actor);
	m_objective = static_cast<SleepObjective*>(objective.get());
	m_actor.m_hasObjectives.addNeed(std::move(objective));
}
void MustSleep::wakeUpEarly()
{
	assert(!m_isAwake);
	assert(m_needsSleep == true);
	m_isAwake = true;
	m_sleepEvent.pause();
	//TODO: partial stamina recovery.
}
void MustSleep::setLocation(Block& block)
{
	m_location = &block;
}
