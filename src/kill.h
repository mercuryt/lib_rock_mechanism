/*
 * Handles getting into position to attack and cooldown.
 * When KillObjective executes it checks if the target is in range and visible.
 * If so and the attacker is not on attack cooldown then do attack and set cooldown.
 * If not then generate a GetIntoRangeAndLineOfSightThreadedTask.
 * Threaded task will call actionComplete after it the route it provides is followed.
 * Also create subscriptions to the target for the publishedEvents Move and Die.
 */

#pragma once

#include "config.h"
#include "deserilizationMemo.h"
#include "objective.h"
#include "eventSchedule.h"
#include "simulation.h"
#include "threadedTask.h"
#include "actor.h"
#include "fight.h"

class Actor;

class KillObjective final : public Objective
{
	Actor& m_killer;
	Actor& m_target;
	HasThreadedTask<GetIntoAttackPositionThreadedTask> m_getIntoRangeAndLineOfSightThreadedTask;
public:
	KillObjective(Actor& k, Actor& t) : Objective(k, Config::killPriority), m_killer(k), m_target(t), m_getIntoRangeAndLineOfSightThreadedTask(k.getThreadedTaskEngine()) { }
	KillObjective(const Json& data, DeserilizationMemo& deserilizationMemo) : Objective(data, deserilizationMemo), 
	m_killer(deserilizationMemo.m_simulation.getActorById(data["killer"].get<ActorId>())), 
	m_target(deserilizationMemo.m_simulation.getActorById(data["target"].get<ActorId>())), 
	m_getIntoRangeAndLineOfSightThreadedTask(deserilizationMemo.m_simulation.m_threadedTaskEngine)
	{ 
		if(data["threadedTask"])
			m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target, m_killer.m_canFight.getMaxRange());
	}
	void execute();
	void cancel() { m_getIntoRangeAndLineOfSightThreadedTask.maybeCancel(); }
	void delay() { cancel(); }
	void reset();
	std::string name() const { return "kill"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Kill; }
};
