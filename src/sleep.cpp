// Sleep Event.
SleepEvent::SleepEvent(uint32_t step, MustSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
void SleepEvent::execute(){ m_needsSleep.wakeUp(); }
~SleepEvent::SleepEvent(){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(uint32_t step, MustSleep& ns) : ScheduledEventWithPercent(step), m_needsSleep(ns) { }
void TiredEvent::execute(){ m_needsSleep.tired(); }
~TiredEvent::TiredEvent(){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Threaded Task.
SleepThreadedTask::SleepThreadedTask(SleepObjective& so) : m_sleepObjective(so) { }
void SleepThreadedTask::readStep()
{
	auto& actor = m_sleepObjective.m_needsSleep.m_actor;
	assert(m_sleepObjective.m_needsSleep.m_location == nullptr);
	Block* outdoorCandidate = nullptr;
	Block* indoorCandidate = nullptr;
	auto condition = [&](Block* block)
	{
		uint32_t desire = m_sleepObjective.desireToSleepAt(*block);
		if(desire == 3)
			return true;
		else if(indoorCandidate == nullptr && desire == 2)
			indoorCandidate = block;	
		else if(outdoorCandidate == nullptr && desire == 1)
			outdoorCandidate = block;	
		return false;
	};
	m_result = path::getForActorToPredicate(actor, condition);
	if(m_result.empty())
		if(indoorCandidate != nullptr)
			m_result = path::getForActorTo(indoorCandidate);
		else if(outdoorCandidate != nullptr)
			m_result = path::getForActorTo(outdoorCandidate);
}
void SleepThreadedTask::writeStep()
{
	auto& actor = m_sleepObjective.m_needsSleep.m_actor;
	if(m_result.empty())
		m_actor.m_hasObjectives.cannotCompleteNeed(m_sleepObjective);
	else
		m_actor.setPath(m_result);
}
// Sleep Objective.
SleepObjective::SleepObjective(MustSleep& ns) : Objective(Config::sleepObjectivePriority), m_needsSleep(ns) { }
void SleepObjective::execute()
{
	if(m_needsSleep.m_actor.m_location == m_needsSleep.m_location)
	{
		assert(m_needsSleep.m_location != nullptr);
		m_needsSleep.sleep();
	}
	else if(m_needsSleep.m_location == nullptr)
		m_threadedTask.create(*this);
	else
		m_needsSleep.m_actor.setDestination(*m_location);
}
uint32_t SleepObjective::desireToSleepAt(Block& block)
{
	if(block->m_reservable.isFullyReserved() || !block->m_temperature.isSafeFor(m_needsSleep.actor))
		return 0;
	if(block->m_outdoors)
		return 1;
	if(block->m_indoors)
		return 2;
	if(block->m_area->m_hasSleepingSpots.contains(block))
		return 3;
}
~SleepObjective::SleepObjective() { m_needsSleep.m_objective = nullptr; }
// Needs Sleep.
MustSleep::MustSleep(Actor& a) : m_actor(a), m_needsSleep(false), m_isAwake(true)
{
	m_tiredEvent.schedule(m_actor.getStepsMustSleepFrequency(), *this);
}
void MustSleep::tired()
{
	assert(m_isAwake);
	if(m_needsSleep)
		sleep();
	else
	{
		m_needsSleep = true;
		m_tiredEvent.schedule(m_actor.getStepsTillSleepOveride(), *this);
		makeSleepObjective();
	}
}
void MustSleep::sleep()
{
	assert(m_isAwake);
	m_isAwake = false;
	m_tiredEvent.unschedule();
	m_sleepEvent.schedule(m_actor.getStepsSleepDuration(), *this);
}
void MustSleep::wakeUp()
{
	assert(!m_isAwake);
	m_isAwake = true;
	m_tiredEvent.schedule(m_actor.getStepsMustSleepFrequency(), *this);
}
void MustSleep::makeSleepObjective()
{
	assert(m_isAwake);
	assert(m_objective == nullptr);
	std::unique_ptr<Objective> objective = std::make_unique<SleepObjective>(*this);
	m_objective = objective.get();
	m_actor.m_hasObjectives.addNeed(objective);
}
void MustSleep::wakeUpEarly()
{
	assert(!m_isAwake);
	assert(m_needsSleep == true);
	m_isAwake = true;
	m_sleepEvent.pause();
}
