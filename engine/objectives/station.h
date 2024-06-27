#pragma once
#include "objective.h"
#include "config.h"
#include "types.h"
class Actor;
struct DeserializationMemo;
class StationObjective final : public Objective
{
	BlockIndex m_location = BLOCK_INDEX_MAX;
public:
	StationObjective(ActorIndex a, BlockIndex l) : Objective(a, Config::stationPriority), m_location(l) { }
	StationObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area);
	void cancel(Area&) { }
	void delay(Area&) { }
	void reset(Area& area);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Station; }
	std::string name() const { return "station"; }
};
/*
class StationInputAction final : public InputAction
{
public:
	BlockIndex& m_block;
	StationInputAction(std::unordered_set<Actor*> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, BlockIndex& b);
	void execute();
};
*/
