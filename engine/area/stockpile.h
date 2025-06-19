#pragma once
//TODO: Move implimentations into cpp file and replace includes with forward declarations it imporove compile time.
#include "../geometry/cuboid.h"
#include "../eventSchedule.h"
#include "../input.h"
#include "../reference.h"
#include "../reservable.h"
#include "../project.h"
#include "../types.h"
#include "../vectorContainers.h"
#include "../projects/stockpile.h"

#include <memory>
#include <sys/types.h>
#include <vector>
#include <utility>
#include <tuple>
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
class StockPile
{
	std::vector<ItemQuery> m_queries;
	SmallSet<BlockIndex> m_blocks;
	Quantity m_openBlocks = Quantity::create(0);
	Area& m_area;
	FactionId m_faction;
	bool m_enabled = true;
	HasScheduledEvent<ReenableStockPileScheduledEvent> m_reenableScheduledEvent;
	StockPileProject* m_projectNeedingMoreWorkers = nullptr;
public:
	StockPile(std::vector<ItemQuery>& q, Area& a, const FactionId& f);
	StockPile(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void addBlock(const BlockIndex& block);
	void removeBlock(const BlockIndex& block);
	void updateQueries(std::vector<ItemQuery>& queries);
	void incrementOpenBlocks();
	void decrementOpenBlocks();
	void disableIndefinately() { m_enabled = false; }
	void disableTemporarily(Step duration) { m_reenableScheduledEvent.schedule(*this, duration); disableIndefinately(); }
	void reenable() { m_enabled = true; }
	void addToProjectNeedingMoreWorkers(const ActorIndex& actor, StockPileObjective& objective);
	void destroy();
	void removeQuery(const ItemQuery& query);
	void addQuery(const ItemQuery& query);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool accepts(const ItemIndex& item) const;
	[[nodiscard]] bool contains(ItemQuery& query) const;
	[[nodiscard]] bool isEnabled() const { return m_enabled; }
	[[nodiscard]] bool hasProjectNeedingMoreWorkers() const;
	[[nodiscard]] Simulation& getSimulation();
	[[nodiscard]] bool contains(const BlockIndex& block) const { return m_blocks.contains(block); }
	[[nodiscard]] std::vector<ItemQuery>& getQueries() { return m_queries; }
	[[nodiscard]] FactionId getFaction() { return m_faction; }
	friend class AreaHasStockPilesForFaction;
	friend class ReenableStockPileScheduledEvent;
	friend class StockPileProject;
	[[nodiscard]] bool operator==(const StockPile& other) const { return &other == this; }
};
class ReenableStockPileScheduledEvent final : public ScheduledEvent
{
	StockPile& m_stockPile;
public:
	ReenableStockPileScheduledEvent(StockPile& sp, const Step& duration, const Step start = Step::null()) : ScheduledEvent(sp.getSimulation(), duration, start), m_stockPile(sp) { }
	void execute(Simulation&, Area*) { m_stockPile.reenable(); }
	void clearReferences(Simulation&, Area*) { m_stockPile.m_reenableScheduledEvent.clearPointer(); }
};
struct BlockIsPartOfStockPile
{
	StockPile* stockPile;
	bool active = false;
};
class BlockIsPartOfStockPiles
{
	SmallMap<FactionId, BlockIsPartOfStockPile> m_stockPiles;
	BlockIndex m_block;
public:
	BlockIsPartOfStockPiles(const BlockIndex& b): m_block(b) { }
	void recordMembership(StockPile& stockPile);
	void recordNoLongerMember(StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void updateActive();
	[[nodiscard]] StockPile* getForFaction(const FactionId& faction) { if(!m_stockPiles.contains(faction)) return nullptr; return m_stockPiles[faction].stockPile; }
	[[nodiscard]] bool contains(const FactionId& faction) const { return m_stockPiles.contains(faction); }
	[[nodiscard]] bool isAvalible(const FactionId& faction) const;
	friend class AreaHasStockPilesForFaction;
};
struct StockPileHasShapeDishonorCallback final : public DishonorCallback
{
	StockPileProject& m_stockPileProject;
	StockPileHasShapeDishonorCallback(StockPileProject& hs) : m_stockPileProject(hs) { }
	StockPileHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const { return Json({{"type", "StockPileHasShapeDishonorCallback"}, {"project", reinterpret_cast<uintptr_t>(&m_stockPileProject)}}); }
	void execute([[maybe_unused]] const Quantity& oldCount, [[maybe_unused]] const Quantity& newCount);
};
class AreaHasStockPilesForFaction
{
	// Stockpiles may accept multiple item types and thus may appear here more then once.
	SmallMap<ItemTypeId, SmallSet<StockPile*>> m_availableStockPilesByItemType;
	// These items are checked whenever a new stockpile is created to see if they should be move to items with destinations.
	SmallMap<ItemTypeId, SmallSet<ItemReference>> m_itemsWithoutDestinationsByItemType;
	// Only when an item is added here does it get designated for stockpileing.
	SmallSet<ItemReference> m_itemsToBeStockPiled;
	//TODO: Replace these SmallMaps with medium maps?
	// The stockpile used as index here is not neccesarily where the item will go, it is used to prove that there is somewhere the item could go.
	SmallMap<StockPile*, SmallSet<ItemReference>> m_itemsWithDestinationsByStockPile;
	// Multiple projects per item due to generic item stacking.
	SmallMap<ItemReference, SmallSet<StockPileProject*>> m_projectsByItem;
	std::list<StockPileProject> m_projects;
	std::list<StockPile> m_stockPiles;
	Area& m_area;
	FactionId m_faction;
	// To be called when the last block is removed from the stockpile.
	// To remove all blocks call StockPile::destroy.
	void destroyStockPile(StockPile& stockPile);
public:
	AreaHasStockPilesForFaction(Area& a, const FactionId& f) : m_area(a), m_faction(f) { }
	AreaHasStockPilesForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const FactionId& f);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	StockPile& addStockPile(std::vector<ItemQuery>&& queries);
	StockPile& addStockPile(std::vector<ItemQuery>& queries);
	void addItem(const ItemIndex& item);
	void maybeAddItem(const ItemIndex& item);
	void removeItem(const ItemIndex& item);
	void removeBlock(const BlockIndex& block);
	void setAvailable(StockPile& stockPile);
	void setUnavailable(StockPile& stockPile);
	void makeProject(const ItemIndex& item, const BlockIndex& destination, StockPileObjective& objective, const ActorIndex& actor);
	void cancelProject(StockPileProject& project);
	void destroyProject(StockPileProject& project);
	void removeFromProjectsByItem(StockPileProject& project);
	void addQuery(StockPile& stockPile, const ItemQuery& query);
	void removeQuery(StockPile& stockPile, const ItemQuery& query);
	void maybeRemoveFromItemsWithDestinationByStockPile(const StockPile& stockpile, const ItemIndex& item);
	void updateItemReferenceForProject(StockPileProject& project, const ItemReference& ref);
	[[nodiscard]] bool isValidStockPileDestinationFor(const BlockIndex& block, const ItemIndex& item) const;
	[[nodiscard]] bool isAnyHaulingAvailableFor(const ActorIndex& actor) const;
	[[nodiscard]] ItemIndex getHaulableItemForAt(const ActorIndex& actor, const BlockIndex& block);
	[[nodiscard]] StockPile* getStockPileFor(const ItemIndex& item) const;
	friend class StockPilePathRequest;
	friend class StockPileDestinationPathRequest;
	friend class StockPile;
	friend class AreaHasStockPiles;
	// For testing.
	[[maybe_unused, nodiscard]] auto& getItemsWithDestinations() { return m_itemsToBeStockPiled; }
	[[maybe_unused, nodiscard]] auto& getItemsWithDestinationsByStockPile() { return m_itemsWithDestinationsByStockPile; }
	[[maybe_unused, nodiscard]] Quantity getItemsWithProjectsCount() { return Quantity::create(m_projectsByItem.size()); }
};
class AreaHasStockPiles
{
	Area& m_area;
	SmallMapStable<FactionId, AreaHasStockPilesForFaction> m_data;
public:
	AreaHasStockPiles(Area& a) : m_area(a) { }
	void registerFaction(const FactionId& faction) { assert(!m_data.contains(faction)); m_data.emplace(faction, m_area, faction); }
	void unregisterFaction(const FactionId& faction) { assert(m_data.contains(faction)); m_data.erase(faction); }
	void removeItemFromAllFactions(const ItemIndex& item) { for(auto& pair : m_data) { pair.second->removeItem(item); } }
	void removeBlockFromAllFactions(const BlockIndex& block) { for(auto& pair : m_data) { pair.second->removeBlock(block); }}
	void clearReservations();
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] AreaHasStockPilesForFaction& getForFaction(const FactionId& faction);
	[[nodiscard]] bool contains(const FactionId& faction) const { return m_data.contains(faction); }
};