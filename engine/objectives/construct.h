#pragma once
#include "../objective.h"
#include "../numericTypes/types.h"
#include "../path/pathRequest.h"
#include "../dataStructures/smallSet.h"
class Project;
class Area;
class ConstructProject;
struct PathResult;
// TODO: specalize construct objective into carpentry, masonry, etc.
class ConstructObjectiveType final : public ObjectiveType
{
public:
	ConstructObjectiveType() = default;
	ConstructObjectiveType(const Json&, DeserializationMemo&){ }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::string name() const { return "construct"; }
};
class ConstructObjective final : public Objective
{
	Project* m_project = nullptr;
	SmallSet<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	ConstructObjective() : Objective(Config::constructObjectivePriority) { }
	ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void joinProject(ConstructProject& project, ActorIndex actor);
	void onProjectCannotReserve(Area& area, ActorIndex actor);
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("construct").getId(); }
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "construct"; }
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAdjacentTo(Area& area, const Point3D location, const Facing4 facing, ActorIndex actor);
	[[nodiscard]] const ConstructProject* getProjectWhichActorCanJoinAdjacentTo(Area& area, const Point3D location, const Facing4 facing, ActorIndex actor) const;
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAt(Area& area, const Cuboid cuboid, ActorIndex actor);
	[[nodiscard]] const ConstructProject* getProjectWhichActorCanJoinAt(Area& area, const Cuboid cuboid, ActorIndex actor) const;
	[[nodiscard]] Point3D joinableProjectExistsAt(Area& area, const Cuboid cuboid, ActorIndex actor) const;
	[[nodiscard]] bool canJoinProjectAdjacentToLocationAndFacing(Area& area, const Point3D point, const Facing4 facing, ActorIndex actor) const;
	friend class ConstructPathRequest;
	friend class ConstructProject;
	// For Testing.
	[[nodiscard, maybe_unused]] Project* getProject() { return m_project; }
};
class ConstructPathRequest final : public PathRequest
{
	ConstructObjective& m_constructObjective;
public:
	ConstructPathRequest(Area& area, ConstructObjective& co, ActorIndex actorIndex);
	ConstructPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] Json toJson() const override;
	[[nodiscard]] std::string name() { return "construct"; }
};