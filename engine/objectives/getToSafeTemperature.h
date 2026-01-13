#pragma once

#include "../numericTypes/types.h"
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../config/config.h"

class Area;
struct DeserializationMemo;
struct FindPathResult;
class GetToSafeTemperatureObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { std::unreachable(); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { std::unreachable(); }
	GetToSafeTemperatureObjectiveType() = default;
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
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("get to safe temperature").getId(); }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::temperature; }
	friend class GetToSafeTemperaturePathRequest;
};
class GetToSafeTemperaturePathRequest final : public PathRequestBreadthFirst
{
	GetToSafeTemperatureObjective& m_objective;
public:
	GetToSafeTemperaturePathRequest(Area& area, GetToSafeTemperatureObjective& objective, const ActorIndex& actorIndex);
	GetToSafeTemperaturePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] Json toJson() const;
};
