#pragma once

#include "../types.h"
#include "../objective.h"
#include "../pathRequest.h"
#include "../config.h"

class Area;
struct DeserializationMemo;
struct FindPathResult;

class GetToSafeTemperatureObjective final : public Objective
{
	bool m_noWhereWithSafeTemperatureFound = false;
public:
	GetToSafeTemperatureObjective() : Objective(Config::getToSafeTemperaturePriority) { }
	GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	friend class GetToSafeTemperaturePathRequest;
};
class GetToSafeTemperaturePathRequest final : public PathRequest
{
	GetToSafeTemperatureObjective& m_objective;
public:
	GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& o);
	void callback(Area& area, FindPathResult& result);
};
