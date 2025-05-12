#pragma once

#include "../objective.h"
#include "../eventSchedule.hpp"
#include "../path/pathRequest.h"
#include "../reservable.h"
#include "../path/terrainFacade.h"
#include "../types.h"
#include "../projects/givePlantFluid.h"

#include <memory>

class GivePlantsFluidObjective;
struct DeserializationMemo;
struct FindPathResult;

// Path to an empty water proof container or somewhere to fill an empty container or a container with the correct type of fluid or a plant which needs fluid.
class GivePlantsFluidPathRequest final : public PathRequestBreadthFirst
{
	GivePlantsFluidObjective& m_objective;
public:
	GivePlantsFluidPathRequest(Area& area, GivePlantsFluidObjective& objective, const ActorIndex& actor);
	GivePlantsFluidPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "give plants fluid"; }
};
class GivePlantsFluidObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	GivePlantsFluidObjectiveType() = default;
	GivePlantsFluidObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
	[[nodiscard]] std::string name() const { return "give plants fluid"; }
};
class GivePlantsFluidObjective final : public Objective
{
	std::unique_ptr<GivePlantFluidProject> m_project;
public:
	GivePlantsFluidObjective() : Objective(Config::givePlantsFluidPriority) { }
	GivePlantsFluidObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area& area, const ActorIndex& actor) override;
	void reset(Area& area, const ActorIndex& actor) override;
	void delay(Area& area, const ActorIndex& actor) override;
	void selectPlantLocation(Area& area, const BlockIndex& block, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	void onBeforeUnload(Area&, const ActorIndex&) override;
	[[nodiscard]] std::string name() const override { return "give plants fluid"; }
	//For testing.
	[[nodiscard]] bool hasProject() const { return m_project != nullptr; }
	[[nodiscard]] const GivePlantFluidProject& getProject() const { return *m_project; }
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class GivePlantsFluidPathRequest;
};