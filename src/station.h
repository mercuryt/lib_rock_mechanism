#pragma once
#include "deserializationMemo.h"
#include "objective.h"
#include "config.h"
#include "simulation.h"
class Block;
class Actor;
class StationObjective final : public Objective
{
	Block& m_location;
public:
	StationObjective(Actor& a, Block& l) : Objective(a, Config::stationPriority), m_location(l) { }
	StationObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_location(deserializationMemo.m_simulation.getBlockForJsonQuery(data["block"])) { }
	void execute();
	void cancel() { }
	void delay() { }
	void reset();
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Station; }
	std::string name() const { return "station"; }
};
