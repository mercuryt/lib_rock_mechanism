#pragma once
#include "../pathRequest.h"
#include "../objective.h"
#include "../types.h"
class Area;
class InstallItemObjective;
class InstallItemProject;
class InstallItemObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	std::string name() const { return "install Item"; }
};
class InstallItemObjective final : public Objective
{
	InstallItemProject* m_project;
public:
	InstallItemObjective();
	InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor); 
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area&, const ActorIndex&) { }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "install item"; }
	[[nodiscard]] Json toJson() const;
	friend class InstallItemPathRequest;
};
// Search for nearby item that needs installing.
class InstallItemPathRequest final : public PathRequestDepthFirst
{
	InstallItemObjective& m_installItemObjective;
public:
	InstallItemPathRequest(Area& area, InstallItemObjective& iio, const ActorIndex& actor);
	InstallItemPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoDepthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() const { return "install item"; }
	[[nodiscard]] Json toJson() const;
};
