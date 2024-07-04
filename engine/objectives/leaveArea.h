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
	LeaveAreaObjective(ActorIndex a, uint8_t priority);
	// No need to overide default to/from json.
	void execute(Area&);
	void cancel(Area&) { }
	void delay(Area&) { }
	void reset(Area&) { }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::LeaveArea; }
	[[nodiscard]] std::string name() const { return "leave area"; }
	[[nodiscard]] bool actorIsAdjacentToEdge() const;
};
class LeaveAreaPathRequest final : public PathRequest
{
	LeaveAreaObjective& m_objective;
public:
	LeaveAreaPathRequest(Area& area, LeaveAreaObjective& objective);
	void callback(Area& aera, FindPathResult& result);
};
