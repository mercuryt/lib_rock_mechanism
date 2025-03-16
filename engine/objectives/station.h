#pragma once
#include "objective.h"
#include "config.h"
#include "types.h"
struct DeserializationMemo;
class StationObjective final : public Objective
{
	BlockIndex m_location;
public:
	StationObjective(const BlockIndex& l, const Priority& priority = Config::stationPriority) : Objective(priority), m_location(l) { }
	StationObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex&) { }
	void delay(Area&, const ActorIndex&) { }
	void reset(Area& area, const ActorIndex& actor);
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
