#pragma once

#include "../objective.h"
#include "../config.h"
#include "../eventSchedule.hpp"
#include "../pathRequest.h"
#include "../types.h"

struct DeserializationMemo;
class HarvestEvent;

class HarvestObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	HarvestObjectiveType() = default;
	HarvestObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::wstring name() const override { return L"harvest"; }
};
class HarvestObjective final : public Objective
{
	BlockIndex m_block;
public:
	HasScheduledEvent<HarvestEvent> m_harvestEvent;
	HarvestObjective(Area& area);
	HarvestObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void select(Area& area, const BlockIndex& block, const ActorIndex& actor);
	void begin(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canHarvestAt(Area& area, const BlockIndex& block) const;
	[[nodiscard]] std::wstring name() const override { return L"harvest"; }
	[[nodiscard]] BlockIndex getBlockContainingPlantToHarvestAtLocationAndFacing(Area& area, const BlockIndex& location, Facing4 facing, const ActorIndex& actor);
	[[nodiscard]] bool blockContainsHarvestablePlant(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	friend class HarvestEvent;
	// For testing.
	BlockIndex getBlock() { return m_block; }
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
	[[nodiscard]] std::wstring name() { return L"harvest"; }
	[[nodiscard]] Json toJson() const override;
};
