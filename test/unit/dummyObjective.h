#pragma once
#include "../../engine/objective.h"
class DummyObjective final : public Objective
{
public:
	DummyObjective() : Objective(Priority::create(1)) { }
	void execute(Area&, const ActorIndex&) { std::unreachable(); }
	void cancel(Area&, const ActorIndex&) { std::unreachable(); }
	void delay(Area&, const ActorIndex&) { std::unreachable(); }
	void reset(Area&, const ActorIndex&) { std::unreachable(); }
	std::wstring name() const { return L"dummy"; }
};
