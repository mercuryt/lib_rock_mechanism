#pragma once

#include "../numericTypes/types.h"
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../config/config.h"

class Area;
struct DeserializationMemo;
struct PathResult;
class GetToSafeTemperatureObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex) const { std::unreachable(); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex) const { std::unreachable(); }
	GetToSafeTemperatureObjectiveType() = default;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
};
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
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("get to safe temperature").getId(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::temperature; }
	friend class GetToSafeTemperaturePathRequest;
};
class GetToSafeTemperaturePathRequest final : public PathRequest
{
	GetToSafeTemperatureObjective& m_objective;
public:
	GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& objective, ActorIndex actorIndex);
	GetToSafeTemperaturePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] Json toJson() const;
};
