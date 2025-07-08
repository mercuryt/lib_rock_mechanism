#pragma once
#include "../objective.h"
#include "../numericTypes/types.h"
#include "../path/pathRequest.h"
#include "../dataStructures/smallSet.h"
class Project;
class Area;
class ConstructProject;
struct FindPathResult;
// TODO: specalize construct objective into carpentry, masonry, etc.
class ConstructObjectiveType final : public ObjectiveType
{
public:
	ConstructObjectiveType() = default;
	ConstructObjectiveType(const Json&, DeserializationMemo&){ }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::string name() const { return "construct"; }
};
class ConstructObjective final : public Objective
{
	Project* m_project = nullptr;
	SmallSet<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	ConstructObjective() : Objective(Config::constructObjectivePriority) { }
	ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void joinProject(ConstructProject& project, const ActorIndex& actor);
	void onProjectCannotReserve(Area& area, const ActorIndex& actor);
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "construct"; }
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAdjacentTo(Area& area, const Point3D& location, const Facing4& facing, const ActorIndex& actor);
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAt(Area& area, const Point3D& point, const ActorIndex& actor);
	[[nodiscard]] bool joinableProjectExistsAt(Area& area, const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool canJoinProjectAdjacentToLocationAndFacing(Area& area, const Point3D& point, const Facing4& facing, const ActorIndex& actor) const;
	friend class ConstructPathRequest;
	friend class ConstructProject;
	// For Testing.
	[[nodiscard, maybe_unused]] Project* getProject() { return m_project; }
};
class ConstructPathRequest final : public PathRequestBreadthFirst
{
	ConstructObjective& m_constructObjective;
public:
	ConstructPathRequest(Area& area, ConstructObjective& co, const ActorIndex& actor);
	ConstructPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const override;
	[[nodiscard]] std::string name() { return "construct"; }
};