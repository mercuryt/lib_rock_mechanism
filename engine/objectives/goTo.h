#pragma once
#include "objective.h"
#include "config.h"
#include "input.h"
#include "types.h"
#include "index.h"

struct DeserializationMemo;
class Area;

class GoToObjective final : public Objective
{
	BlockIndex m_location;
public:
	GoToObjective(BlockIndex l) : Objective(Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area, ActorIndex actor);
	void cancel(Area&, ActorIndex) { }
	void delay(Area&, ActorIndex) { }
	void reset(Area&, ActorIndex) { }
	std::string name() const { return "go to"; }
	static void create(BlockIndex block);
	// For testing.
	[[maybe_unused]] BlockIndex getLocation() { return m_location; }
};
class GoToInputAction final : public InputAction
{
public:
	BlockIndex m_block;
	GoToInputAction(ItemIndices actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex b);
	void execute();
};
