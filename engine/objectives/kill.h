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

struct DeserializationMemo;

class KillInputAction final : public InputAction
{
	ActorIndex m_killer;
	ActorIndex m_target;
	KillInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, ActorIndex killer, ActorIndex target);
	void execute();
};

class KillObjective final : public Objective
{
	ActorIndex m_killer;
	ActorIndex m_target;
public:
	KillObjective(ActorIndex k, ActorIndex t);
	KillObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	std::string name() const { return "kill"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Kill; }
};
