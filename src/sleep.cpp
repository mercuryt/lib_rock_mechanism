#include "sleep.h"
#include "area.h"
#include "path.h"
#include <cassert>
// Sleep Event.
SleepEvent::SleepEvent(uint32_t step, MustSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
void SleepEvent::execute(){ m_needsSleep.wakeUp(); }
SleepEvent::~SleepEvent(){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(uint32_t step, MustSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
void TiredEvent::execute(){ m_needsSleep.tired(); }
TiredEvent::~TiredEvent(){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Threaded Task.
SleepThreadedTask::SleepThreadedTask(SleepObjective& so) : m_sleepObjective(so) { }
void SleepThreadedTask::readStep()
{
	auto& actor = m_sleepObjective.m_actor;
	assert(m_sleepObjective.m_actor.m_mustSleep.m_location == nullptr);
	Block* outdoorCandidate = nullptr;
	Block* indoorCandidate = nullptr;
	auto condition = [&](Block& block)
	{
		uint32_t desire = m_sleepObjective.desireToSleepAt(block);
		if(desire == 3)
			return true;
		else if(indoorCandidate == nullptr && desire == 2)
			indoorCandidate = &block;	
		else if(outdoorCandidate == nullptr && desire == 1)
			outdoorCandidate = &block;	
		return false;
	};
	m_result = path::getForActorToPredicate(actor, condition);
	if(m_result.empty())
	{
		if(indoorCandidate != nullptr)
			m_result = path::getForActor(actor, *indoorCandidate);
		else if(outdoorCandidate != nullptr)
			m_result = path::getForActor(actor, *outdoorCandidate);
	}
}
void SleepThreadedTask::writeStep()
{
	auto& actor = m_sleepObjective.m_actor;
	if(m_result.empty())
		actor.m_hasObjectives.cannotFulfillNeed(m_sleepObjective);
	else
		actor.m_canMove.setPath(m_result);
}
// Sleep Objective.
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
uint32_t SleepObjective::desireToSleepAt(Block& block)
{
	if(block.m_reservable.isFullyReserved() || !m_actor.m_needsSafeTemperature.isSafe(block.m_blockHasTemperature.get()))
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
MustSleep::MustSleep(Actor& a) : m_actor(a), m_location(nullptr), m_isAwake(true)
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
	m_tiredEvent.unschedule();
	m_sleepEvent.schedule(m_actor.m_species.stepsSleepDuration, *this);
}
void MustSleep::wakeUp()
{
	assert(!m_isAwake);
	m_isAwake = true;
	m_tiredEvent.schedule(m_actor.m_species.stepsSleepFrequency, *this);
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
}
