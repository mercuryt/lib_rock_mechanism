#pragma once
#include "../../engine/objective.h"
class DummyObjective final : public Objective
{
public:
	DummyObjective() : Objective(Priority::create(1)) { }
	void execute(Area&, const ActorIndex) override { std::unreachable(); }
	void cancel(Area&, const ActorIndex) override { std::unreachable(); }
	void delay(Area&, const ActorIndex) override { std::unreachable(); }
	void reset(Area&, const ActorIndex) override { std::unreachable(); }
	std::string name() const { return "dummy"; }
};
