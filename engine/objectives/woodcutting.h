#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "../dataStructures/smallSet.h"
#include "numericTypes/index.h"
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
	SmallSet<WoodCuttingProject*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	WoodCuttingObjective();
	WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor);
	void onProjectCannotReserve(Area& area, const ActorIndex& actor);
	void joinProject(WoodCuttingProject& project, const ActorIndex& index);
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("woodcutting").getId(); }
	[[nodiscard]] std::string name() const { return "woodcutting"; }
	[[nodiscard]] WoodCuttingProject* getJoinableProjectAt(Area& area, const Point3D& point, const ActorIndex& index);
	[[nodiscard]] Point3D joinableProjectExistsAt(Area &area, const Cuboid& cuboid, const ActorIndex& actor) const;
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class WoodCuttingPathRequest;
	friend class WoodCuttingProject;
};
// Find a place to woodCutting.
class WoodCuttingPathRequest final : public PathRequestBreadthFirst
{
	WoodCuttingObjective& m_woodCuttingObjective;
	// Result is the point which will be the actors location while doing the woodCuttingging.
public:
	WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective, const ActorIndex& actor);
	WoodCuttingPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "woodcutting"; }
};
