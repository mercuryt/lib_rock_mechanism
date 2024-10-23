#pragma once
#include "config.h"
#include "objective.h"
#include "pathRequest.h"
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
class LeaveAreaPathRequest final : public PathRequest
{
	LeaveAreaObjective& m_objective;
public:
	LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective);
	LeaveAreaPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, const FindPathResult& result);
	[[nodiscard]] std::string name() const { return "leave area"; }
	[[nodiscard]] Json toJson() const;
};
