#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
class Project;
class Area;
class ConstructProject;
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
	ConstructObjective(ActorIndex a);
	ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area);
	void reset(Area& area);
	void joinProject(ConstructProject& project);
	void onProjectCannotReserve(Area& area);
	[[nodiscard]] std::string name() const { return "construct"; }
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAdjacentTo(Area& area, BlockIndex location, Facing facing);
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAt(Area& area, BlockIndex block);
	[[nodiscard]] bool joinableProjectExistsAt(Area& area, BlockIndex block) const;
	[[nodiscard]] bool canJoinProjectAdjacentToLocationAndFacing(Area& area, BlockIndex block, Facing facing) const;
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
	ConstructPathRequest(Area& area, ConstructObjective& co);
	void callback(Area& area, FindPathResult& result);
};
