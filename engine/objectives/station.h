#pragma once
#include "objective.h"
#include "config.h"
#include "types.h"
class Actor;
struct DeserializationMemo;
class StationObjective final : public Objective
{
	BlockIndex m_location;
public:
	StationObjective(BlockIndex l) : Objective(Config::stationPriority), m_location(l) { }
	StationObjective(const Json& data);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area&, ActorIndex) { }
	void delay(Area&, ActorIndex) { }
	void reset(Area& area, ActorIndex actor);
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
