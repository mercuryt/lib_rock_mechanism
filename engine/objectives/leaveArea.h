#pragma once
#include "config.h"
#include "objective.h"
#include "path/pathRequest.h"
struct DeserializationMemo;
class LeaveAreaPathRequest;
struct FindPathResult;
class LeaveAreaObjective final : public Objective
{
public:
	LeaveAreaObjective(Priority priority);
	// No need to overide default to/from json.
	void execute(Area&, const ActorIndex&);
	void cancel(Area&, const ActorIndex&) { }
	void delay(Area&, const ActorIndex&) { }
	void reset(Area&, const ActorIndex&) { }
	[[nodiscard]] std::string name() const { return "leave area"; }
};
class LeaveAreaPathRequest final : public PathRequestBreadthFirst
{
	LeaveAreaObjective& m_objective;
public:
	LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective, const ActorIndex& actorIndex);
	LeaveAreaPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() const { return "leave area"; }
	[[nodiscard]] Json toJson() const;
};
