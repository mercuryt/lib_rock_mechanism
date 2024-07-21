#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
#include "../config.h"
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
	DigProject* m_project = nullptr;
	std::unordered_set<DigProject*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	DigObjective() : Objective(Config::digObjectivePriority) { }
	DigObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void onProjectCannotReserve(Area& area, ActorIndex actor);
	void joinProject(DigProject& project, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
	[[nodiscard]] DigProject* getJoinableProjectAt(Area& area, BlockIndex block, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "dig"; }
	friend class DigPathRequest;
	friend class DigProject;
	//For testing.
	[[nodiscard]] DigProject* getProject() { return m_project; }
};
// Find a place to dig.
class DigPathRequest final : public PathRequest
{
	DigObjective& m_digObjective;
	// Result is the block which will be the actors location while doing the digging.
public:
	DigPathRequest(Area& area, DigObjective& digObjective, ActorIndex actor);
	void callback(Area& area, FindPathResult& result);
	void clearReferences(Simulation&, Area* area);
};
