#pragma once
#include "../../engine/objective.h"
class DummyObjective final : public Objective
{
public:
	DummyObjective() : Objective(Priority::create(1)) { }
	void execute(Area&, ActorIndex) { assert(false); }
	void cancel(Area&, ActorIndex) { assert(false); }
	void delay(Area&, ActorIndex) { assert(false); }
	void reset(Area&, ActorIndex) { assert(false); }
	std::string name() const { return "dummy"; }

};