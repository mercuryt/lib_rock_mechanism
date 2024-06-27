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
	GetToSafeTemperatureObjective(ActorIndex a);
	GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GetToSafeTemperature; }
	std::string name() const { return "get to safe temperature"; }
	~GetToSafeTemperatureObjective();
	friend class GetToSafeTemperaturePathRequest;
};
class GetToSafeTemperaturePathRequest final : public PathRequest
{
	GetToSafeTemperatureObjective& m_objective;
public:
	GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& o);
	void callback(Area& area, FindPathResult result);
};
