#pragma once

#include "../objective.h"
#include "../terrainFacade.h"
#include "../pathRequest.h"
#include "../types.h"

struct DeserializationMemo;
class Area;
class WanderObjective;

class WanderPathRequest final : public PathRequest
{
	WanderObjective& m_objective;
	uint16_t m_blockCounter = 0;
public:
	WanderPathRequest(Area& area, WanderObjective& objective);
	void callback(Area& area, FindPathResult& result);
};
class WanderObjective final : public Objective
{
public:
	WanderObjective(ActorIndex a);
	WanderObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	std::string name() const { return "wander"; }
	[[nodiscard]] bool canResume() const { return false; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Wander; }
	// For testing.
	[[nodiscard]] bool hasPathRequest(const Area& area) const;
};
