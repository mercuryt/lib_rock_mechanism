/*
 * Handles getting into position to attack and cooldown.
 * When KillObjective executes it checks if the target is in range and visible.
 * If so and the attacker is not on attack cooldown then do attack and set cooldown.
 * If not then generate a GetIntoRangeAndLineOfSightThreadedTask.
 * Threaded task will call actionComplete after it the route it provides is followed.
 * Also create subscriptions to the target for the publishedEvents Move and Die.
 */

#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"

template<class Actor>
class AttackCoolDown : public ScheduledEvent
{
	Actor& m_actor;
	AttackCoolDown(uint32_t step, Actor& a) : ScheduledEvent(step), m_actor(a) {}
	void execute()
	{
		m_actor.m_attackCooldown = false;
		m_actor.actionComplete();
	}
	~AttackCoolDown() { m_actor.m_attackCooldownEvent = nullptr; }
};
template<class KillObjective>
class KillObjectiveOnTargetMove : EventSubscription
{
	KillObjective& m_killObjective;
	KillObjectiveOnTargetMove(KillObjective& ko) : m_killObjective(ko) {}
	void execute() 
	{ 
		// If the kill objective is at the top of the task queue.
		if(m_killObjective.m_killer.m_objectiveQueue.top().get() == &m_killObjective)
			// Do attack or get new position to attack from.
			m_killObjective.execute(); 
	}
	~KillObjectiveOnTargetMove(){ m_killObjective.m_targetMoveEventSubscription = nullptr; }
};
template<class KillObjective>
class KillObjectiveOnTargetDeath : EventSubscription
{
	KillObjective& m_killObjective;
	KillObjectiveOnTargetMove(KillObjective& ko) : m_killObjective(ko) {}
	void execute() { m_killObjective.m_killer.objectiveComplete(); }
	~KillObjectiveOnTargetMove(){ m_killObjective.m_targetDieEventSubscription = nullptr; }
};
template<class Actor>
class KillObjective : public Objective
{
public:
	Actor& m_killer;
	Actor& m_target;
	HasThreadedTask<GetInToRangeAndLineOfSightThreadedTask> m_getIntoRangeAndLineOfSightThreadedTask;
	SubscribesToEvent<KillObjectiveOnTargetDeath> m_targetDeathEventSubscription;
	SubscribesToEvent<KillObjectiveOnTargetMove> m_targetMoveEventSubscription;
	KillObjective(Actor& k, Actor& t) : Objective(Config::killPriority), m_killer(k), m_target(t) { }
	void execute()
	{
		if(!m_target.m_alive)
			//TODO: Do we need to cancel the threaded task here?
			m_killer.objectiveComplete();
		if(m_targetMoveEventSubscription.empty())
			m_targetMoveEventSubscription.subscribe(*this);
		// If not in range create GetIntoRangeThreadedTask.
		if(m_targetDeathEventSubscription.empty())
			m_targetDeathEventSubscription.subscribe(*this);
		if(m_getIntoRangeAndLineOfSightThreadedTask.empty() && m_killer.m_location->taxiDistance(m_target.location) > m_killer.m_maxRange)
			m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target.m_location, m_killer.m_maxAttackRange);
		else
			// If in range and attack not on cooldown then attack.
			if(!m_killer.m_attackCooldown)
				attack(m_killer, m_target);
	}
};
