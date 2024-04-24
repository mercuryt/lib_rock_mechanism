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
#include "input.h"
#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "fight.h"

class Actor;
struct DeserializationMemo;

class KillInputAction final : public InputAction
{
	Actor& m_killer;
	Actor& m_target;
	KillInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Actor& killer, Actor& target);
	void execute();
};

class KillObjective final : public Objective
{
	Actor& m_killer;
	Actor& m_target;
	HasThreadedTask<GetIntoAttackPositionThreadedTask> m_getIntoRangeAndLineOfSightThreadedTask;
public:
	KillObjective(Actor& k, Actor& t);
	KillObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel() { m_getIntoRangeAndLineOfSightThreadedTask.maybeCancel(); }
	void delay() { cancel(); }
	void reset();
	std::string name() const { return "kill"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Kill; }
};
