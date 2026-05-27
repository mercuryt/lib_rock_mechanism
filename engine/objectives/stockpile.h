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
	bool canBeAssigned(Area& area, const ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, const ActorIndex actor) const;
	StockPileObjectiveType() = default;
	[[nodiscard]] std::string name() const { return "stockpile"; }
};
class StockPileObjective final : public Objective
{
	std::vector<std::tuple<ItemTypeId, MaterialTypeId>> m_closedList;
	Point3D m_stockPileLocation;
	Point3D m_itemStartLocation;
	// Item type is stored because we alredy know it will fit at location.
	// Material type, quality, etc. are not stored because the stockpile will check those via 'accepts'.
	ItemTypeId m_itemType;
	bool m_hasCheckedForCloserDropOffLocation = false;
public:
	StockPileProject* m_project = nullptr;
	StockPileObjective();
	StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void execute(Area& area, const ActorIndex actor);
	void cancel(Area& area, const ActorIndex actor);
	void delay(Area& area, const ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex actor);
	void setItemAndDestination(Area& area, ItemIndex item, Point3D destination);
	void unsetItemAndDestination();
	[[nodiscard]] ObjectiveTypeId getTypeId() const override { return ObjectiveType::getByName("stockpile").getId(); }
	[[nodiscard]] bool destinationCondition(Area& area, const Point3D point, const ItemIndex item, const ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] constexpr std::string name() const { return "stockpile"; }
	// For debug.
	[[nodiscard]] bool hasItem() const { return m_itemStartLocation.exists(); }
	[[nodiscard]] bool hasDestination() const { return m_stockPileLocation.exists(); }
	[[nodiscard]] const Point3D getDestination() const { return m_stockPileLocation; }
	[[nodiscard]] ItemIndex getItem(const Area& area, ActorIndex actor) const;
	[[nodiscard]] bool canBeAddedToPrioritySet() { return true; }
	friend class StockPilePathRequest;
	friend class StockPileDestinationPathRequest;
};
// Searches for an Item and destination to make or find a hauling project for m_objective.m_actor.
class StockPilePathRequest final : public PathRequest
{
	StockPileObjective& m_objective;
	// One set of candidates for each type. When a new candidate is found compare it to all of the other type and if no match store it.
	// No need to serialize these, they exist only for a single read step.
	SmallMap<StockPile*, CuboidSet> m_pointsByStockPile;
	SmallSet<ItemIndex> m_items;
public:
	StockPilePathRequest(Area& area, StockPileObjective& spo, const ActorIndex actor);
	StockPilePathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] bool checkDestination(const Area& area, const ItemIndex item, const Point3D point) const;
	[[nodiscard]] std::string name() const { return "stockpile"; }
	[[nodiscard]] Json toJson() const;
};
// Attempts to find a closer stockpile destination for a selected item.
// The first pass StockPilePathRequest is efficent for finding an item and a destination but it may not be the best destination.
// With the knowledge that some destination exists we can select the item and then seek the best destination.
class StockPileDestinationPathRequest final : public PathRequest
{
	StockPileObjective& m_objective;
public:
	StockPileDestinationPathRequest(Area& area, StockPileObjective& spo, const ActorIndex actor);
	StockPileDestinationPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	PathResult readStep(Area& area, const AreaHasPathsForMoveType& hasPaths) override;
	void writeStep(Area& area, bool useCurrentLocation) override;
	[[nodiscard]] std::string name() const { return "stockpile destination"; }
	[[nodiscard]] Json toJson() const;
};