#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../vectorContainers.h"
class Area;
class WoodCuttingProject;
// ObjectiveType
class WoodCuttingObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	[[nodiscard]] std::string name() const { return "woodcutting"; }
};
// Objective
class WoodCuttingObjective final : public Objective
{
	WoodCuttingProject* m_project;
	SmallSet<Project*> m_cannotJoinWhileReservationsAreNotComplete;
public:
	WoodCuttingObjective();
	WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor);
	void onProjectCannotReserve(Area& area, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "woodcutting"; }
	void joinProject(WoodCuttingProject& project, ActorIndex index);
	WoodCuttingProject* getJoinableProjectAt(Area& area, BlockIndex block, ActorIndex index);
	friend class WoodCuttingPathRequest;
	friend class WoodCuttingProject;
};
// Find a place to woodCutting.
class WoodCuttingPathRequest final : public PathRequest
{
	WoodCuttingObjective& m_woodCuttingObjective;
	// Result is the block which will be the actors location while doing the woodCuttingging.
public:
	WoodCuttingPathRequest(Area& area, WoodCuttingObjective& woodCuttingObjective);
	WoodCuttingPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, FindPathResult& result);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "woodcutting"; }
};
