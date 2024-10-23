#pragma once

#include "../types.h"
#include "../objective.h"
#include "../pathRequest.h"
#include "../config.h"

class Area;
struct DeserializationMemo;
struct FindPathResult;
class GetToSafeTemperatureObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { assert(false); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { assert(false); }
	GetToSafeTemperatureObjectiveType() = default;
	GetToSafeTemperatureObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
};
class GetToSafeTemperatureObjective final : public Objective
{
	bool m_noWhereWithSafeTemperatureFound = false;
public:
	GetToSafeTemperatureObjective() : Objective(Config::getToSafeTemperaturePriority) { }
	GetToSafeTemperatureObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::temperature; }
	friend class GetToSafeTemperaturePathRequest;
};
class GetToSafeTemperaturePathRequest final : public PathRequest
{
	GetToSafeTemperatureObjective& m_objective;
public:
	GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& o, const ActorIndex& actor);
	GetToSafeTemperaturePathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult& result);
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] Json toJson() const;
};
