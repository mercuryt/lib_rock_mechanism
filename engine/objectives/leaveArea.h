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
	LeaveAreaObjective(int8_t priority);
	// No need to overide default to/from json.
	void execute(Area&, ActorIndex);
	void cancel(Area&, ActorIndex) { }
	void delay(Area&, ActorIndex) { }
	void reset(Area&, ActorIndex) { }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::LeaveArea; }
	[[nodiscard]] std::string name() const { return "leave area"; }
};
class LeaveAreaPathRequest final : public PathRequest
{
	LeaveAreaObjective& m_objective;
public:
	LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective);
	void callback(Area& aera, FindPathResult& result);
};
