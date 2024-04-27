#pragma once
#include "objective.h"
#include "config.h"
#include "input.h"

class Block;
class Actor;
struct DeserializationMemo;

class GoToObjective final : public Objective
{
	Block& m_location;
public:
	GoToObjective(Actor& a, Block& l) : Objective(a, Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel() { }
	void delay() { }
	void reset() { }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GoTo; }
	std::string name() const { return "go to"; }
	static void create(Actor& actor, Block& block);
	// For testing.
	[[maybe_unused]] const Block& getLocation() { return m_location; }
};
class GoToInputAction final : public InputAction
{
public:
	Block& m_block;
	GoToInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Block& b);
	void execute();
};
