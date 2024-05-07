#include "kill.h"
#include "block.h"
#include "objective.h"
#include "visionUtil.h"
#include "simulation.h"
#include <memory>
KillInputAction::KillInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Actor& killer, Actor& target) : InputAction(actors, emplacementType, inputQueue), m_killer(killer), m_target(target) 
{
       	m_onDestroySubscriptions.subscribe(m_target.m_onDestroy); 
}
void KillInputAction::execute()
{
	std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(m_killer, m_target);
	insertObjective(std::move(objective), m_killer);
}
KillObjective::KillObjective(Actor& k, Actor& t) : Objective(k, Config::killPriority), m_killer(k), m_target(t), m_getIntoRangeAndLineOfSightThreadedTask(k.getThreadedTaskEngine()) { }
KillObjective::KillObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_killer(deserializationMemo.m_simulation.getActorById(data["killer"].get<ActorId>())), 
	m_target(deserializationMemo.m_simulation.getActorById(data["target"].get<ActorId>())), 
	m_getIntoRangeAndLineOfSightThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine)
{ 
	if(data["threadedTask"])
		m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target, m_killer.m_canFight.getMaxRange());
}
void KillObjective::execute()
{
	if(!m_target.isAlive())
		//TODO: Do we need to cancel the threaded task here?
		m_killer.m_hasObjectives.objectiveComplete(*this);
	m_killer.m_canFight.setTarget(m_target);
	// If not in range create GetIntoRangeThreadedTask.
	if(!m_getIntoRangeAndLineOfSightThreadedTask.exists() && 
			(m_killer.m_location->taxiDistance(*m_target.m_location) > m_killer.m_canFight.getMaxRange() ||
			 // TODO: hasLineOfSightIncludingActors
			 visionUtil::hasLineOfSightBasic(*m_killer.m_location, *m_target.m_location))
	)
		m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target, m_killer.m_canFight.getMaxRange());
	else
		// If in range and has line of sight and attack not on cooldown then attack.
		if(!m_killer.m_canFight.isOnCoolDown())
			m_killer.m_canFight.attackMeleeRange(m_target);
}
void KillObjective::reset() 
{ 
	cancel(); 
	m_killer.m_canReserve.deleteAllWithoutCallback(); 
}
