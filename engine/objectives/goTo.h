#pragma once
#include "objective.h"
#include "config.h"
#include "input.h"
#include "types.h"

struct DeserializationMemo;
class Area;

class GoToObjective final : public Objective
{
	BlockIndex m_location = BLOCK_INDEX_MAX;
public:
	GoToObjective(BlockIndex l) : Objective(Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area, ActorIndex actor);
	void cancel(Area&, ActorIndex) { }
	void delay(Area&, ActorIndex) { }
	void reset(Area&, ActorIndex) { }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GoTo; }
	std::string name() const { return "go to"; }
	static void create(BlockIndex block);
	// For testing.
	[[maybe_unused]] BlockIndex getLocation() { return m_location; }
};
class GoToInputAction final : public InputAction
{
public:
	BlockIndex m_block;
	GoToInputAction(std::unordered_set<ActorIndex> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex b);
	void execute();
};
