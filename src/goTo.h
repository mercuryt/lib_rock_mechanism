#pragma once
#include "objective.h"
#include "config.h"
class Block;
class Actor;
class GoToObjective final : public Objective
{
	Actor& m_actor;
	Block& m_location;
public:
	GoToObjective(Actor& a, Block& l) : Objective(Config::goToPriority), m_actor(a), m_location(l) { }
	void execute();
	void cancel() { }
	void delay() { }
	void reset() { }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GoTo; }
	std::string name() const { return "go to"; }
};
