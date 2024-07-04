#pragma once
#include "../objective.h"
#include "../pathRequest.h"

class Area;
class StockPileProject;
struct ItemType;
struct MaterialType;

class StockPileObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::StockPile; }
	StockPileObjectiveType() = default;
	StockPileObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class StockPileObjective final : public Objective
{
	std::vector<std::tuple<const ItemType*, const MaterialType*>> m_closedList;
	ItemIndex m_item = ITEM_INDEX_MAX;
	BlockIndex m_destination = BLOCK_INDEX_MAX;
public:
	StockPileProject* m_project = nullptr;
	StockPileObjective(ActorIndex a);
	StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area) { cancel(area); }
	[[nodiscard]] bool destinationCondition(Area& area, BlockIndex block, const ItemIndex item);
	std::string name() const { return "stockpile"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::StockPile; }
	friend class StockPilePathRequest;
};
// Searches for an Item and destination to make or find a hauling project for m_objective.m_actor.
class StockPilePathRequest final : public PathRequest
{
	StockPileObjective& m_objective;
public:
	StockPilePathRequest(Area& area, StockPileObjective& spo);
	void callback(Area& area, FindPathResult& result);
};
