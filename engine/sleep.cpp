#include "sleep.h"
#include "area.h"
#include "designations.h"
#include "hasShape.h"
#include "objective.h"
#include "simulation.h"
#include <cassert>
// Sleep Event.
SleepEvent::SleepEvent(Step step, MustSleep& ns, bool f, const Step start) : ScheduledEvent(ns.m_actor.getSimulation(), step, start), m_needsSleep(ns), m_force(f) { }
void SleepEvent::execute(){ m_needsSleep.wakeUp(); }
void SleepEvent::clearReferences(){ m_needsSleep.m_sleepEvent.clearPointer(); }
// Tired Event.
TiredEvent::TiredEvent(Step step, MustSleep& ns, const Step start) : ScheduledEvent(ns.m_actor.getSimulation(), step, start), m_needsSleep(ns) { }
void TiredEvent::execute(){ m_needsSleep.tired(); }
void TiredEvent::clearReferences(){ m_needsSleep.m_tiredEvent.clearPointer(); }
// Threaded Task.
SleepThreadedTask::SleepThreadedTask(SleepObjective& so) : ThreadedTask(so.m_actor.getThreadedTaskEngine()), m_sleepObjective(so), m_findsPath(so.m_actor, so.m_detour), m_sleepAtCurrentLocation(false), m_noWhereToSleepFound(false) { }
void SleepThreadedTask::readStep()
{
	auto& actor = m_sleepObjective.m_actor;
	assert(m_sleepObjective.m_actor.m_mustSleep.m_location == nullptr);
	const Block* maxDesireCandidate = nullptr;
	const Block* outdoorCandidate = nullptr;
	const Block* indoorCandidate = nullptr;
	uint32_t desireToSleepAtCurrentLocation = m_sleepObjective.desireToSleepAt(*m_sleepObjective.m_actor.m_location);
	if(desireToSleepAtCurrentLocation == 1)
		outdoorCandidate = m_sleepObjective.m_actor.m_location;
	else if(desireToSleepAtCurrentLocation == 2)
		indoorCandidate = m_sleepObjective.m_actor.m_location;
	else if(desireToSleepAtCurrentLocation == 3)
		maxDesireCandidate = m_sleepObjective.m_actor.m_location;
	if(maxDesireCandidate == nullptr)
	{
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
		if(actor.getFaction())
			m_findsPath.pathToUnreservedAdjacentToPredicate(condition, *actor.getFaction());
		else
			m_findsPath.pathToAdjacentToPredicate(condition);
	}
	if(!m_findsPath.found())
	{
		// If the current location is the max desired then set sleep at current to true.
		if(maxDesireCandidate == actor.m_location)
		{
			assert(maxDesireCandidate != nullptr);
			m_sleepAtCurrentLocation = true;
		}
		// No max desire target found, try to at least get to a safe temperature
		// TODO: paths which have already been previously calculated are being caluclated again here.
		else if(indoorCandidate != nullptr)
		{
			if(indoorCandidate == actor.m_location)
				m_sleepAtCurrentLocation = true;
			else
				m_findsPath.pathToBlock(*indoorCandidate);
		}
		else if(outdoorCandidate != nullptr)
		{
			// sleep outdoors.
			if(outdoorCandidate == actor.m_location)
				m_sleepAtCurrentLocation = true;
			else
				m_findsPath.pathToBlock(*outdoorCandidate);
		}
		else
		{
			// No candidates, try to leave area
			m_noWhereToSleepFound = true;
			m_findsPath.pathToAreaEdge();
		}
	}
}
void SleepThreadedTask::writeStep()
{
	auto& actor = m_sleepObjective.m_actor;
	m_findsPath.cacheMoveCosts();
	if(!m_findsPath.found())
	{
		if(m_sleepAtCurrentLocation)
			actor.m_mustSleep.sleep();
		else
			actor.m_hasObjectives.cannotFulfillNeed(m_sleepObjective);
	}
	else
	{
		if(m_findsPath.areAllBlocksAtDestinationReservable(actor.getFaction()))
		{
			m_findsPath.reserveBlocksAtDestination(actor.m_canReserve);
			actor.m_canMove.setPath(m_findsPath.getPath());
		}
		else
			// Selected sleeping spot reserved by someone else, look again.
			m_sleepObjective.m_threadedTask.create(m_sleepObjective);
	}
	if(m_noWhereToSleepFound)
		m_sleepObjective.m_noWhereToSleepFound = true;
}
void SleepThreadedTask::clearReferences() { m_sleepObjective.m_threadedTask.clearPointer(); }
// Sleep Objective.
SleepObjective::SleepObjective(Actor& a) : Objective(a, Config::sleepObjectivePriority), m_threadedTask(a.getThreadedTaskEngine()), m_noWhereToSleepFound(false) { }
SleepObjective::SleepObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), m_noWhereToSleepFound(data["noWhereToSleepFound"].get<bool>())
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json SleepObjective::toJson() const
{
	Json data = Objective::toJson();
	data["noWhereToSleepFound"] = m_noWhereToSleepFound;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
void SleepObjective::execute()
{
	assert(m_actor.m_mustSleep.m_isAwake);
	if(m_actor.m_mustSleep.m_location == nullptr)
	{
		if(m_noWhereToSleepFound)
		{
			if(m_actor.predicateForAnyOccupiedBlock([](const Block& block){ return block.m_isEdge; }))
				// We are at the edge and can leave.
				m_actor.leaveArea();
			else
				// No sleep and no escape.
				m_actor.m_hasObjectives.cannotFulfillNeed(*this);
			return;
		}
		else
			m_threadedTask.create(*this);
	}
	else if(m_actor.m_location == m_actor.m_mustSleep.m_location)
	{
		if(desireToSleepAt(*m_actor.m_location) == 0)
		{
			// Can not sleep here any more, look for another spot.
			m_actor.m_mustSleep.m_location = nullptr;
			execute();
		}
		else
			// Sleep.
			m_actor.m_mustSleep.sleep(); 
	}
	else
		if(m_actor.m_mustSleep.m_location->m_hasShapes.canEnterEverWithAnyFacing(m_actor))
			m_actor.m_canMove.setDestination(*m_actor.m_mustSleep.m_location, m_detour);
		else
		{
			// Location no longer can be entered. 
			m_actor.m_mustSleep.m_location = nullptr;
			execute();
		}
}
uint32_t SleepObjective::desireToSleepAt(const Block& block) const
{
	if(block.m_reservable.isFullyReserved(m_actor.getFaction()) || !m_actor.m_needsSafeTemperature.isSafe(block.m_blockHasTemperature.get()))
		return 0;
	if(block.m_area->m_hasSleepingSpots.containsUnassigned(block))
		return 3;
	if(block.m_outdoors)
		return 1;
	else
		return 2;
}
void SleepObjective::cancel() 
{ 
	m_threadedTask.maybeCancel();
	m_actor.m_canReserve.deleteAllWithoutCallback(); 
}
void SleepObjective::reset() 
{ 
	cancel(); 
	m_noWhereToSleepFound = false; 
}

bool SleepObjective::onNoPath() 
{ 
	if(m_actor.m_mustSleep.m_location == nullptr)
		return false;
	m_actor.m_mustSleep.m_location = nullptr; 
	execute(); 
	return true;
}
SleepObjective::~SleepObjective() { m_actor.m_mustSleep.m_objective = nullptr; }
// Needs Sleep.
MustSleep::MustSleep(Actor& a) : m_actor(a), m_location(nullptr), m_sleepEvent(a.getEventSchedule()), m_tiredEvent(a.getEventSchedule()), m_objective(nullptr), m_needsSleep(false), m_isAwake(true)
{
	m_tiredEvent.schedule(m_actor.m_species.stepsSleepFrequency, *this);
}
MustSleep::MustSleep(const Json data, Actor& a) : 
	m_actor(a), m_location(data.contains("location") ? &a.getSimulation().getBlockForJsonQuery(data["location"]) : nullptr), 
	m_sleepEvent(a.getEventSchedule()), m_tiredEvent(a.getEventSchedule()), m_objective(nullptr), 
	m_needsSleep(data["needsSleep"].get<bool>()), m_isAwake(data["isAwake"].get<bool>())
{
	if(data.contains("sleepEventStart"))
		m_sleepEvent.schedule(m_actor.m_species.stepsSleepDuration, *this, data["sleepEventStart"].get<Step>());
	if(data.contains("tiredEventStart"))
		m_tiredEvent.schedule(m_actor.m_species.stepsSleepFrequency, *this, data["tiredEventStart"].get<Step>());
}
Json MustSleep::toJson() const
{
	Json data;
	if(m_location != nullptr)
		data["location"] = m_location->positionToJson();
	data["needsSleep"] = m_needsSleep;
	data["isAwake"] = m_isAwake;
	if(m_sleepEvent.exists())
		data["sleepEventStart"] = m_sleepEvent.getStartStep();
	if(m_tiredEvent.exists())
		data["tiredEventStart"] = m_tiredEvent.getStartStep();
	return data;
}
void MustSleep::notTired()
{
	assert(m_isAwake);
	if(m_objective != nullptr)
		m_actor.m_hasObjectives.objectiveComplete(*m_objective);
	m_needsSleep = false;
	m_tiredEvent.unschedule();
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
		m_tiredEvent.maybeUnschedule();
		m_tiredEvent.schedule(m_actor.m_species.stepsTillSleepOveride, *this);
		makeSleepObjective();
	}
}
// Voulentary sleep.
void MustSleep::sleep() { sleep(m_actor.m_species.stepsSleepDuration); }
// Involuntary sleep.
void MustSleep::passout(Step duration) { sleep(duration, true); }
void MustSleep::sleep(Step duration, bool force)
{
	assert(m_isAwake);
	m_actor.m_canMove.clearAllEventsAndTasks();
	m_isAwake = false;
	m_tiredEvent.maybeUnschedule();
	m_sleepEvent.schedule(duration, *this, force);
	if(m_objective != nullptr)
		m_objective->m_threadedTask.maybeCancel();
}
void MustSleep::wakeUp()
{
	assert(!m_isAwake);
	m_isAwake = true;
	m_needsSleep = false;
	m_tiredEvent.schedule(m_actor.m_species.stepsSleepFrequency, *this);
	m_actor.m_stamina.setFull();
	// Objective complete releases all reservations.
	if(m_objective != nullptr)
		m_actor.m_hasObjectives.objectiveComplete(*m_objective);
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
	m_tiredEvent.schedule(m_actor.m_species.stepsTillSleepOveride, *this);
	//TODO: partial stamina recovery.
}
void MustSleep::setLocation(Block& block)
{
	m_location = &block;
}
void MustSleep::onDeath()
{
	m_tiredEvent.maybeUnschedule();
}
void HasSleepingSpots::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& block : data["unassigned"])
		m_unassigned.insert(&deserializationMemo.m_simulation.getBlockForJsonQuery(block));
}
Json HasSleepingSpots::toJson() const
{
	Json data{{"unassigned", Json::array()}};
	for(Block* block : m_unassigned)
		data["unassigned"].push_back(block);
	return data;
}
void HasSleepingSpots::designate(const Faction& faction, Block& block)
{
	m_unassigned.insert(&block);
	block.m_hasDesignations.insert(faction, BlockDesignation::Sleep);
}
void HasSleepingSpots::undesignate(const Faction& faction, Block& block)
{
	m_unassigned.erase(&block);
	block.m_hasDesignations.remove(faction, BlockDesignation::Sleep);
}
