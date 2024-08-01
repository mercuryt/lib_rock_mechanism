#pragma once
//TODO: Move implimentations into cpp file and replace includes with forward declarations it imporove compile time.
#include "cuboid.h"
#include "eventSchedule.h"
#include "input.h"
#include "reference.h"
#include "reservable.h"
#include "project.h"
#include "types.h"
#include "vectorContainers.h"

#include <memory>
#include <sys/types.h>
#include <vector>
#include <utility>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <list>

class StockPilePathRequest;
class StockPileProject;
class Simulation;
class ReenableStockPileScheduledEvent;
class StockPile;
class BlockIsPartOfStockPiles;
struct DeserializationMemo;
struct MaterialType;
struct FindPathResult;
class Area;
class StockPileObjective;

/*
class StockPileCreateInputAction final : public InputAction
{
	Cuboid m_cuboid;
	FactionId m_faction;
	std::vector<ItemQuery> m_queries;
	StockPileCreateInputAction(InputQueue& inputQueue, Cuboid& cuboid, FactionId faction, std::vector<ItemQuery>& queries) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction), m_queries(queries) { }
	void execute();
};
class StockPileRemoveInputAction final : public InputAction
{
	Cuboid m_cuboid;
	FactionId m_faction;
	StockPileRemoveInputAction(InputQueue& inputQueue, Cuboid& cuboid, FactionId faction ) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction) { }
	void execute();
};
class StockPileExpandInputAction final : public InputAction
{
	Cuboid m_cuboid;
	StockPile& m_stockpile;
	StockPileExpandInputAction(InputQueue& inputQueue, Cuboid& cuboid, StockPile& stockpile) : InputAction(inputQueue), m_cuboid(cuboid), m_stockpile(stockpile) { }
	void execute();
};
class StockPileUpdateInputAction final : public InputAction
{
	StockPile& m_stockpile;
	std::vector<ItemQuery> m_queries;
	StockPileUpdateInputAction(InputQueue& inputQueue, StockPile& stockpile, std::vector<ItemQuery>& queries) : InputAction(inputQueue), m_stockpile(stockpile), m_queries(queries) { }
	void execute();
};
*/
class StockPile
{
	std::vector<ItemQuery> m_queries;
	BlockIndices m_blocks;
	Quantity m_openBlocks = Quantity::create(0);
	Area& m_area;
	FactionId m_faction;
	bool m_enabled = true;
	HasScheduledEvent<ReenableStockPileScheduledEvent> m_reenableScheduledEvent;
	StockPileProject* m_projectNeedingMoreWorkers = nullptr;
public:
	StockPile(std::vector<ItemQuery>& q, Area& a, FactionId f);
	StockPile(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void addBlock(BlockIndex block);
	void removeBlock(BlockIndex block);
	void updateQueries(std::vector<ItemQuery>& queries);
	void incrementOpenBlocks();
	void decrementOpenBlocks();
	void disableIndefinately() { m_enabled = false; }
	void disableTemporarily(Step duration) { m_reenableScheduledEvent.schedule(*this, duration);  disableIndefinately(); }
	void reenable() { m_enabled = true; } 
	void addToProjectNeedingMoreWorkers(ActorIndex actor, StockPileObjective& objective);
	void destroy();
	void removeQuery(ItemQuery& query);
	void addQuery(ItemQuery& query);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool accepts(const ItemIndex item) const;
	[[nodiscard]] bool contains(ItemQuery& query) const;
	[[nodiscard]] bool isEnabled() const { return m_enabled; }
	[[nodiscard]] bool hasProjectNeedingMoreWorkers() const;
	[[nodiscard]] Simulation& getSimulation();
	[[nodiscard]] bool contains(BlockIndex block) const { return m_blocks.contains(block); }
	[[nodiscard]] std::vector<ItemQuery>& getQueries() { return m_queries; }
	[[nodiscard]] FactionId getFaction() { return m_faction; }
	friend class AreaHasStockPilesForFaction;
	friend class ReenableStockPileScheduledEvent;
	friend class StockPileProject;
	[[nodiscard]] bool operator==(const StockPile& other) const { return &other == this; }
};
class StockPileProject final : public Project
{
	ItemReference m_item;
	// Needed for generic items where the original item may no longer exist.
	Quantity m_quantity = Quantity::create(0);
	const ItemType& m_itemType;
	const MaterialType& m_materialType;
	StockPile& m_stockpile;
	void onComplete();
	void onReserve();
	// Mark dispatched as true and dismiss any unassigned workers and candidates.
	void onCancel();
	// TODO: geometric progresson of disable duration.
	void onDelay();
	void offDelay() { assert(false); }
	void onHasShapeReservationDishonored(ActorOrItemIndex actorOrItem, Quantity oldCount, Quantity newCount);
	[[nodiscard]] bool canReset() const { return false; }
	[[nodiscard]] Step getDuration() const { return Config::addToStockPileDelaySteps; }
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, Quantity>> getByproducts() const;
	std::vector<std::pair<ActorQuery, Quantity>> getActors() const;
public:
	StockPileProject(FactionId faction, Area& area, BlockIndex block, ItemIndex item, Quantity quantity, Quantity maxWorkers);
	StockPileProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canAddWorker(const ActorIndex actor) const;
	// Don't recruit more workers then are needed for hauling.
	[[nodiscard]] bool canRecruitHaulingWorkersOnly() const { return true; }
	friend class AreaHasStockPilesForFaction;
	// For testing.
	[[nodiscard, maybe_unused]] ItemIndex getItem() { return m_item.getIndex(); }
};
class ReenableStockPileScheduledEvent final : public ScheduledEvent
{
	StockPile& m_stockPile;
public:
	ReenableStockPileScheduledEvent(StockPile& sp, Step duration, const Step start = Step::create(0)) : ScheduledEvent(sp.getSimulation(), duration, start), m_stockPile(sp) { }
	void execute(Simulation&, Area*) { m_stockPile.reenable(); }
	void clearReferences(Simulation&, Area*) { m_stockPile.m_reenableScheduledEvent.clearPointer(); }
};
struct BlockIsPartOfStockPile
{
	StockPile& stockPile;
	bool active = false;
};
class BlockIsPartOfStockPiles
{
	FactionIdMap<BlockIsPartOfStockPile> m_stockPiles;
	BlockIndex m_block;
public:
	BlockIsPartOfStockPiles(BlockIndex b): m_block(b) { }
	void recordMembership(StockPile& stockPile);
	void recordNoLongerMember(StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void updateActive();
	[[nodiscard]] StockPile* getForFaction(FactionId faction) { if(!m_stockPiles.contains(faction)) return nullptr; return &m_stockPiles.at(faction).stockPile; }
	[[nodiscard]] bool contains(FactionId faction) const { return m_stockPiles.contains(faction); }
	[[nodiscard]] bool isAvalible(FactionId faction) const;
	friend class AreaHasStockPilesForFaction;
};
struct StockPileHasShapeDishonorCallback final : public DishonorCallback
{
	StockPileProject& m_stockPileProject;
	StockPileHasShapeDishonorCallback(StockPileProject& hs) : m_stockPileProject(hs) { } 
	StockPileHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const { return Json({{"type", "StockPileHasShapeDishonorCallback"}, {"project", reinterpret_cast<uintptr_t>(&m_stockPileProject)}}); }
	// Craft step project cannot reset so cancel instead and allow to be recreated later.
	// TODO: Why?
	void execute([[maybe_unused]] Quantity oldCount, [[maybe_unused]] Quantity newCount) { m_stockPileProject.cancel(); }
};
class AreaHasStockPilesForFaction
{
	// Stockpiles may accept multiple item types and thus may appear here more then once.
	std::unordered_map<const ItemType*, std::unordered_set<StockPile*>> m_availableStockPilesByItemType;
	// These items are checked whenever a new stockpile is created to see if they should be move to items with destinations.
	std::unordered_map<const ItemType*, ItemReferences> m_itemsWithoutDestinationsByItemType;
	// Only when an item is added here does it get designated for stockpileing.
	ItemReferences m_itemsWithDestinationsWithoutProjects;
	// The stockpile used as index here is not neccesarily where the item will go, it is used to prove that there is somewhere the item could go.
	std::unordered_map<StockPile*, ItemReferences> m_itemsWithDestinationsByStockPile;
	// Multiple projects per item due to generic item stacking.
	std::unordered_map<ItemReference, std::list<StockPileProject>, ItemReference::Hash> m_projectsByItem;
	std::list<StockPile> m_stockPiles;
	Area& m_area;
	FactionId m_faction;
	// To be called when the last block is removed from the stockpile.
	// To remove all blocks call StockPile::destroy.
	void destroyStockPile(StockPile& stockPile);
public:
	AreaHasStockPilesForFaction(Area& a, FactionId f) : m_area(a), m_faction(f) { }
	AreaHasStockPilesForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, FactionId f);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	StockPile& addStockPile(std::vector<ItemQuery>&& queries);
	StockPile& addStockPile(std::vector<ItemQuery>& queries);
	void addItem(ItemIndex item);
	void maybeAddItem(ItemIndex item);
	void removeItem(ItemIndex item);
	void removeBlock(BlockIndex block);
	void setAvailable(StockPile& stockPile);
	void setUnavailable(StockPile& stockPile);
	void makeProject(ItemIndex item, BlockIndex destination, StockPileObjective& objective, ActorIndex actor);
	void cancelProject(StockPileProject& project);
	void destroyProject(StockPileProject& project);
	void addQuery(StockPile& stockPile, ItemQuery query);
	void removeQuery(StockPile& stockPile, ItemQuery query);
	void removeFromItemsWithDestinationByStockPile(const StockPile& stockpile, const ItemIndex item);
	[[nodiscard]] bool isValidStockPileDestinationFor(BlockIndex block, const ItemIndex item) const;
	[[nodiscard]] bool isAnyHaulingAvailableFor(const ActorIndex actor) const;
	[[nodiscard]] ItemIndex getHaulableItemForAt(const ActorIndex actor, BlockIndex block);
	[[nodiscard]] StockPile* getStockPileFor(const ItemIndex item) const;
	friend class StockPilePathRequest;
	friend class StockPile;
	friend class AreaHasStockPiles;
	// For testing.
	[[maybe_unused, nodiscard]] auto& getItemsWithDestinations() { return m_itemsWithDestinationsWithoutProjects; }
	[[maybe_unused, nodiscard]] auto& getItemsWithDestinationsByStockPile() { return m_itemsWithDestinationsByStockPile; }
	[[maybe_unused, nodiscard]] Quantity getItemsWithProjectsCount() { return Quantity::create(m_projectsByItem.size()); }
};
class AreaHasStockPiles
{
	Area& m_area;
	FactionIdMap<AreaHasStockPilesForFaction> m_data;
public:
	AreaHasStockPiles(Area& a) : m_area(a) { }
	void registerFaction(FactionId faction) { assert(!m_data.contains(faction)); m_data.try_emplace(faction, m_area, faction); }
	void unregisterFaction(FactionId faction) { assert(m_data.contains(faction)); m_data.erase(faction); }
	void removeItemFromAllFactions(ItemIndex item) { for(auto& pair : m_data) { pair.second.removeItem(item); } }
	void removeBlockFromAllFactions(BlockIndex block) { for(auto& pair : m_data) { pair.second.removeBlock(block); }} 
	void clearReservations();
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] AreaHasStockPilesForFaction& at(FactionId faction);
	[[nodiscard]] bool contains(const FactionId faction) const { return m_data.contains(faction); }
};
