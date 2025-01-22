#pragma once
#include "../objective.h"
#include "../types.h"
#include "../pathRequest.h"
#include "../vectorContainers.h"
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
	[[nodiscard]] std::wstring name() const { return L"construct"; }
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
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::wstring name() const { return L"construct"; }
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAdjacentTo(Area& area, const BlockIndex& location, const Facing4& facing, const ActorIndex& actor);
	[[nodiscard]] ConstructProject* getProjectWhichActorCanJoinAt(Area& area, const BlockIndex& block, const ActorIndex& actor);
	[[nodiscard]] bool joinableProjectExistsAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	[[nodiscard]] bool canJoinProjectAdjacentToLocationAndFacing(Area& area, const BlockIndex& block, const Facing4& facing, const ActorIndex& actor) const;
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
	[[nodiscard]] std::wstring name() { return L"construct"; }
};