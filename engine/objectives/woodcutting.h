#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../vectorContainers.h"
#include "index.h"
class Area;
class WoodCuttingProject;
// ObjectiveType
class WoodCuttingObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::string name() const { return "woodcutting"; }
};
// Objective
class WoodCuttingObjective final : public Objective
{
	WoodCuttingProject* m_project = nullptr;
	SmallSet<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	WoodCuttingObjective();
	WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor);
	void onProjectCannotReserve(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "woodcutting"; }
	void joinProject(WoodCuttingProject& project, const ActorIndex& index);
	[[nodiscard]] WoodCuttingProject* getJoinableProjectAt(Area& area, const BlockIndex& block, const ActorIndex& index);
	[[nodiscard]] bool joinableProjectExistsAt(Area &area, const BlockIndex& block, const ActorIndex& actor) const;
	friend class WoodCuttingPathRequest;
	friend class WoodCuttingProject;
};
// Find a place to woodCutting.
class WoodCuttingPathRequest final : public PathRequestBreadthFirst
{
	WoodCuttingObjective& m_woodCuttingObjective;
	// Result is the block which will be the actors location while doing the woodCuttingging.
public:
	WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective, const ActorIndex& actor);
	WoodCuttingPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "woodcutting"; }
};
