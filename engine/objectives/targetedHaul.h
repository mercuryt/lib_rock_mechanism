#pragma once

#include "../objective.h"

class Area;
class TargetedHaulProject;
struct DeserializationMemo;

class TargetedHaulObjective final : public Objective
{
	TargetedHaulProject& m_project;
public:
	TargetedHaulObjective(ActorIndex a, TargetedHaulProject& p);
	TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area&, ActorIndex actor);
	void cancel(Area&, ActorIndex actor);
	void delay(Area&, ActorIndex) { }
	void reset(Area& area, ActorIndex actor) { cancel(area, actor); }
	[[nodiscard]] std::string name() const { return "haul"; }
	[[nodiscard]] Json toJson() const;
};
