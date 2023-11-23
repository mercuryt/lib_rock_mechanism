#pragma once
#include "objective.h"
#include "config.h"
class Block;
class Actor;
class StationObjective final : public Objective
{
	Actor& m_actor;
	Block& m_location;
public:
	StationObjective(Actor& a, Block& l) : Objective(Config::stationPriority), m_actor(a), m_location(l) { }
	void execute();
	void cancel() { }
	void delay() { }
	void reset();
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Station; }
	std::string name() const { return "station"; }
};
