#pragma once
#include "../objective.h"
#include "../path/pathRequest.h"
#include "numericTypes/types.h"

class Area;
class StockPileProject;
struct ItemType;
struct MaterialType;

class StockPileObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, const ActorIndex& actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex& actor) const;
	StockPileObjectiveType() = default;
	[[nodiscard]] std::string name() const { return "stockpile"; }
};
class StockPileObjective final : public Objective
{
	std::vector<std::tuple<ItemTypeId, MaterialTypeId>> m_closedList;
	ItemReference m_item;
	Point3D m_stockPileLocation;
	// Store where the actor will stand when picking up the item, for use when calculating the best drop off.
	Point3D m_pickUpLocation;
	Facing4 m_pickUpFacing;
	bool m_hasCheckedForCloserDropOffLocation = false;
public:
	StockPileProject* m_project = nullptr;
	StockPileObjective();
	StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("stockpile").getId(); }
	[[nodiscard]] bool destinationCondition(Area& area, const Point3D& point, const ItemIndex& item, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] constexpr std::string name() const { return "stockpile"; }
	// For debug.
	[[nodiscard]] bool hasItem() const { return m_item.exists(); }
	[[nodiscard]] bool hasDestination() const { return m_stockPileLocation.exists(); }
	[[nodiscard]] const Point3D& getDestination() const { return m_stockPileLocation; }
	[[nodiscard]] ItemReference getItem() const { return m_item; }
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class StockPilePathRequest;
	friend class StockPileDestinationPathRequest;
};
// Searches for an Item and destination to make or find a hauling project for m_objective.m_actor.
class StockPilePathRequest final : public PathRequestBreadthFirst
{
	StockPileObjective& m_objective;
	// One set of candidates for each type. When a new candidate is found compare it to all of the other type and if no match store it.
	// No need to serialize these, they exist only for a single read step.
	SmallMap<StockPile*, SmallSet<Point3D>> m_pointsByStockPile;
	SmallSet<ItemIndex> m_items;
public:
	StockPilePathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actor);
	StockPilePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] bool checkDestination(const Area& area, const ItemIndex& item, const Point3D& point) const;
	[[nodiscard]] std::string name() const { return "stockpile"; }
	[[nodiscard]] Json toJson() const;
};
// Attempts to find a closer stockpile destination for a selected item.
// The first pass StockPilePathRequest is efficent for finding an item and a destination but it may not be the best destination.
// With the knowledge that some destination exists we can select the item and then seek the best destination.
class StockPileDestinationPathRequest final : public PathRequestBreadthFirst
{
	StockPileObjective& m_objective;
public:
	StockPileDestinationPathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actor);
	StockPileDestinationPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] std::string name() const { return "stockpile destination"; }
	[[nodiscard]] Json toJson() const;
};