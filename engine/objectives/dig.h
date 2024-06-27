#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
class Area;
class DigProject;
class DigObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	DigObjectiveType() = default;
	DigObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class DigObjective final : public Objective
{
	Project* m_project = nullptr;
	std::unordered_set<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	DigObjective(ActorIndex a);
	DigObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area);
	void reset(Area& area);
	void onProjectCannotReserve(Area& area);
	void joinProject(DigProject& project);
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	[[nodiscard]] DigProject* getJoinableProjectAt(Area& area, BlockIndex block);
	[[nodiscard]] std::string name() const { return "dig"; }
	friend class DigPathRequest;
	friend class DigProject;
	//For testing.
	[[nodiscard]] Project* getProject() { return m_project; }
};
// Find a place to dig.
class DigPathRequest final : public PathRequest
{
	DigObjective& m_digObjective;
	// Result is the block which will be the actors location while doing the digging.
public:
	DigPathRequest(Area& area, DigObjective& digObjective);
	void callback(Area& area, FindPathResult result);
	void clearReferences(Simulation&, Area* area);
};
