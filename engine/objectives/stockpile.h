#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "types.h"

class Area;
class StockPileProject;
struct ItemType;
struct MaterialType;

class StockPileObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	StockPileObjectiveType() = default;
	[[nodiscard]] std::string name() const { return "stockpile"; }
};
class StockPileObjective final : public Objective
{
	std::vector<std::tuple<ItemTypeId, MaterialTypeId>> m_closedList;
	ItemReference m_item;
	BlockIndex m_destination;
public:
	StockPileProject* m_project = nullptr;
	StockPileObjective();
	StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor) { cancel(area, actor); }
	[[nodiscard]] bool destinationCondition(Area& area, BlockIndex block, const ItemIndex item, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	std::string name() const { return "stockpile"; }
	friend class StockPilePathRequest;
};
// Searches for an Item and destination to make or find a hauling project for m_objective.m_actor.
class StockPilePathRequest final : public PathRequest
{
	StockPileObjective& m_objective;
public:
	StockPilePathRequest(Area& area, StockPileObjective& spo);
	StockPilePathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, FindPathResult& result);
	[[nodiscard]] std::string name() const { return "stockpile"; }
	[[nodiscard]] Json toJson() const;
};
