#pragma once

#include "../objective.h"
#include "../config.h"
#include "../eventSchedule.hpp"
#include "../path/pathRequest.h"
#include "../numericTypes/types.h"

struct DeserializationMemo;
class HarvestEvent;

class HarvestObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	HarvestObjectiveType() = default;
	HarvestObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const override { return "harvest"; }
};
class HarvestObjective final : public Objective
{
	Point3D m_point;
public:
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HarvestObjective(Area& area);
	HarvestObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void select(Area& area, const Point3D& point, const ActorIndex& actor);
	void begin(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canHarvestAt(Area& area, const Point3D& point) const;
	[[nodiscard]] std::string name() const override { return "harvest"; }
	[[nodiscard]] Point3D getPointContainingPlantToHarvestAtLocationAndFacing(Area& area, const Point3D& location, Facing4 facing, const ActorIndex& actor);
	[[nodiscard]] bool pointContainsHarvestablePlant(Area& area, const Point3D& point, const ActorIndex& actor) const;
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class HarvestEvent;
	// For testing.
	Point3D getPoint() { return m_point; }
};
class HarvestEvent final : public ScheduledEvent
{
	ActorReference m_actor;
	HarvestObjective& m_harvestObjective;
public:
	HarvestEvent(const Step& delay, Area& area, HarvestObjective& ho, const ActorIndex& actor, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
	PlantIndex getPlant();
};
class HarvestPathRequest final : public PathRequestBreadthFirst
{
	HarvestObjective& m_objective;
public:
	HarvestPathRequest(Area& area, HarvestObjective& objective, const ActorIndex& actor);
	HarvestPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() { return "harvest"; }
	[[nodiscard]] Json toJson() const override;
};
