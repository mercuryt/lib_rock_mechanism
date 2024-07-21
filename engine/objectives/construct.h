#pragma once
#include "../objective.h"
#include "../types.h"
#include "../pathRequest.h"
#include "config.h"
class Project;
class Area;
class ConstructProject;
struct FindPathResult;
// TODO: specalize construct objective into carpentry, masonry, etc.
class ConstructObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Construct; }
	ConstructObjectiveType() = default;
	ConstructObjectiveType(const Json&, DeserializationMemo&){ }
};
class ConstructObjective final : public Objective
{
	Project* m_project = nullptr;
	std::unordered_set<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	ConstructObjective() : Objective(Config::constructObjectivePriority) { }
	ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void joinProject(ConstructProject& project, ActorIndex actor);
	void onProjectCannotReserve(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "construct"; }
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAdjacentTo(Area& area, BlockIndex location, Facing facing, ActorIndex actor);
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAt(Area& area, BlockIndex block, ActorIndex actor);
	[[nodiscard]] bool joinableProjectExistsAt(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] bool canJoinProjectAdjacentToLocationAndFacing(Area& area, BlockIndex block, Facing facing, ActorIndex actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Construct; }
	friend class ConstructPathRequest;
	friend class ConstructProject;
	// For Testing.
	[[nodiscard, maybe_unused]] Project* getProject() { return m_project; }
};
class ConstructPathRequest final : public PathRequest
{
	ConstructObjective& m_constructObjective;
public:
	ConstructPathRequest(Area& area, ConstructObjective& co, ActorIndex actor);
	void callback(Area& area, FindPathResult& result);
};
