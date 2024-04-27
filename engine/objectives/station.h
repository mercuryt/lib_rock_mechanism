#pragma once
#include "objective.h"
#include "config.h"
class Block;
class Actor;
struct DeserializationMemo;
class StationObjective final : public Objective
{
	Block& m_location;
public:
	StationObjective(Actor& a, Block& l) : Objective(a, Config::stationPriority), m_location(l) { }
	StationObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel() { }
	void delay() { }
	void reset();
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Station; }
	std::string name() const { return "station"; }
};
/*
class StationInputAction final : public InputAction
{
public:
	Block& m_block;
	StationInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, Block& b);
	void execute();
};
*/
