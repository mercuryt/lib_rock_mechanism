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
#include "actor.h"
#include "getIntoAttackPosition.h"

class KillObjective final : public Objective
{
public:
	Actor& m_killer;
	Actor& m_target;
	HasThreadedTask<GetIntoRangeAndLineOfSightThreadedTask> m_getIntoRangeAndLineOfSightThreadedTask;
	KillObjective(Actor& k, Actor& t) : Objective(Config::killPriority), m_killer(k), m_target(t) { }
	void execute()
	{
		if(!m_target.m_alive)
			//TODO: Do we need to cancel the threaded task here?
			m_killer.objectiveComplete();
		m_killer.m_canFight.setTarget(m_target);
		// If not in range create GetIntoRangeThreadedTask.
		if(m_getIntoRangeAndLineOfSightThreadedTask.empty() && m_killer.m_location->taxiDistance(m_target.location) > m_killer.m_maxRange)
			m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target.m_location, m_killer.m_maxAttackRange);
		else
			// If in range and attack not on cooldown then attack.
			if(!m_killer.m_attackCooldown)
				attack(m_killer, m_target);
	}
};
