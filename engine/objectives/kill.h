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
//#include "input.h"
#include "objective.h"
#include "eventSchedule.h"
#include "threadedTask.h"
#include "index.h"

struct DeserializationMemo;
/*
class KillInputAction final : public InputAction
{
	ActorIndex m_killer;
	ActorIndex m_target;
	KillInputAction(SmallSet<ActorIndex> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, const ActorIndex& killer, const ActorIndex& target);
	void execute();
};
*/
class KillObjective final : public Objective
{
	ActorReference m_target;
public:
	KillObjective(ActorReference t);
	KillObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	std::string name() const { return "kill"; }
};