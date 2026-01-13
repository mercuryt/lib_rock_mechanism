#pragma once

#include "../numericTypes/types.h"
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../config/config.h"

class Area;
struct DeserializationMemo;
struct FindPathResult;
class FleeObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { return false; }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { std::unreachable(); }
	FleeObjectiveType() = default;
	[[nodiscard]] std::string name() const { return "flee"; }
};
class FleeObjective final : public Objective
{
public:
	FleeObjective() : Objective(Config::fleePriority) { }
	FleeObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("flee").getId(); }
	[[nodiscard]] Json toJson() const { return Objective::toJson(); }
	[[nodiscard]] std::string name() const { return "flee"; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::flee; }
	friend class FleePathRequest;
};
class FleePathRequest final : public PathRequestBreadthFirst
{
	FleeObjective& m_objective;
public:
	FleePathRequest(Area& area, FleeObjective& objective, const ActorIndex& actorIndex);
	FleePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() const { return "get to safe temperature"; }
	[[nodiscard]] Json toJson() const;
};
