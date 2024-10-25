#pragma once
#include "../../engine/objective.h"
class DummyObjective final : public Objective
{
public:
	DummyObjective() : Objective(Priority::create(1)) { }
	void execute(Area&, const ActorIndex&) { assert(false); }
	void cancel(Area&, const ActorIndex&) { assert(false); }
	void delay(Area&, const ActorIndex&) { assert(false); }
	void reset(Area&, const ActorIndex&) { assert(false); }
	std::string name() const { return "dummy"; }
};
