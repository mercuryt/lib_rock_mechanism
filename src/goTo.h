#pragma once
#include "deserilizationMemo.h"
#include "objective.h"
#include "config.h"
class Block;
class Actor;
class GoToObjective final : public Objective
{
	Block& m_location;
public:
	GoToObjective(Actor& a, Block& l) : Objective(a, Config::goToPriority), m_location(l) { }
	GoToObjective(const Json& data, DeserilizationMemo& deserilizationMemo);
	Json toJson() const;
	void execute();
	void cancel() { }
	void delay() { }
	void reset() { }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GoTo; }
	std::string name() const { return "go to"; }
};
