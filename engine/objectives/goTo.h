#pragma once
#include "objective.h"
#include "config.h"
#include "input.h"
#include "types.h"

class Actor;
struct DeserializationMemo;

class GoToObjective final : public Objective
{
	BlockIndex m_location = BLOCK_INDEX_MAX;
public:
	GoToObjective(Actor& a, BlockIndex l) : Objective(a, Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel() { }
	void delay() { }
	void reset() { }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GoTo; }
	std::string name() const { return "go to"; }
	static void create(Actor& actor, BlockIndex block);
	// For testing.
	[[maybe_unused]] BlockIndex getLocation() { return m_location; }
};
class GoToInputAction final : public InputAction
{
public:
	BlockIndex m_block;
	GoToInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex b);
	void execute();
};
