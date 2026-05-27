#pragma once
#include "config/config.h"
#include "objective.h"
#include "path/pathRequest.h"
struct DeserializationMemo;
class LeaveAreaPathRequest;
struct PathResult;
class LeaveAreaObjective final : public Objective
{
public:
	LeaveAreaObjective(Priority priority);
	// No need to overide default to/from json.
	void execute(Area&, const ActorIndex);
	void cancel(Area&, const ActorIndex) { }
	void delay(Area&, const ActorIndex) { }
	void reset(Area&, const ActorIndex) { }
	[[nodiscard]] std::string name() const { return "leave area"; }
};
class LeaveAreaPathRequest final : public PathRequest
{
	LeaveAreaObjective& m_objective;
public:
	LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective, const ActorIndex actor);
	LeaveAreaPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] std::string name() const { return "leave area"; }
	[[nodiscard]] Json toJson() const;
};
