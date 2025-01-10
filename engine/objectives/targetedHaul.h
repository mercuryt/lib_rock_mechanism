#pragma once

#include "../objective.h"

class Area;
class TargetedHaulProject;
struct DeserializationMemo;

class TargetedHaulObjective final : public Objective
{
	TargetedHaulProject& m_project;
public:
	TargetedHaulObjective(TargetedHaulProject& p);
	TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area&, const ActorIndex& actor);
	void cancel(Area&, const ActorIndex& actor);
	void delay(Area&, const ActorIndex&) { }
	void reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	[[nodiscard]] std::wstring name() const { return L"haul"; }
	[[nodiscard]] Json toJson() const;
};
