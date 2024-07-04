#pragma once

#include "../objective.h"

class Area;
class TargetedHaulProject;
class DeserializationMemo;

class TargetedHaulObjective final : public Objective
{
	TargetedHaulProject& m_project;
public:
	TargetedHaulObjective(ActorIndex a, TargetedHaulProject& p);
	TargetedHaulObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area&);
	void cancel(Area&);
	void delay(Area&) { }
	void reset(Area& area) { cancel(area); }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Haul; }
	std::string name() const { return "haul"; }
};
