#pragma once
//TODO: Move implimentations into cpp file and replace includes with forward declarations it imporove compile time.
#include "cuboid.h"
#include "deserializationMemo.h"
#include "eventSchedule.h"
#include "faction.h"
#include "hasShape.h"
#include "haul.h"
#include "input.h"
#include "materialType.h"
#include "objective.h"
#include "reservable.h"
#include "threadedTask.hpp"
#include "project.h"
#include "item.h"
#include "actor.h"
#include "findsPath.h"

#include <memory>
#include <sys/types.h>
#include <vector>
#include <utility>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <list>

class Block;
class StockPileThreadedTask;
class StockPileProject;
class Simulation;
class ReenableStockPileScheduledEvent;
class StockPile;
class BlockIsPartOfStockPiles;
class StockPileCreateInputAction final : public InputAction
{
	Cuboid m_cuboid;
	const Faction& m_faction;
	std::vector<ItemQuery> m_queries;
	StockPileCreateInputAction(InputQueue& inputQueue, Cuboid& cuboid, const Faction& faction, std::vector<ItemQuery>& queries) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction), m_queries(queries) { }
	void execute();
};
class StockPileRemoveInputAction final : public InputAction
{
	Cuboid m_cuboid;
	const Faction& m_faction;
	StockPileRemoveInputAction(InputQueue& inputQueue, Cuboid& cuboid, const Faction& faction ) : InputAction(inputQueue), m_cuboid(cuboid), m_faction(faction) { }
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
class StockPileObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::StockPile; }
	StockPileObjectiveType() = default;
	StockPileObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class StockPileObjective final : public Objective
{
public:
	HasThreadedTask<StockPileThreadedTask> m_threadedTask;
	StockPileProject* m_project;
	StockPileObjective(Actor& a) : Objective(a, Config::stockPilePriority), m_threadedTask(a.getThreadedTaskEngine()), m_project(nullptr) { }
	StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset() { cancel(); }
	std::string name() const { return "stockpile"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::StockPile; }
};
// Searches for an Item and destination to make or find a hauling project for m_objective.m_actor.
class StockPileThreadedTask final : public ThreadedTask
{
	StockPileObjective& m_objective;
	Item* m_item;
	Block* m_destination;
	FindsPath m_findsPath;
public:
	StockPileThreadedTask(StockPileObjective& spo) : ThreadedTask(spo.m_actor.getThreadedTaskEngine()), m_objective(spo), m_item(nullptr), m_destination(nullptr), m_findsPath(spo.m_actor, spo.m_detour) { }
	void readStep();
	void writeStep();
	void clearReferences();
};
class StockPile
{
	std::vector<ItemQuery> m_queries;
	std::unordered_set<Block*> m_blocks;
	uint32_t m_openBlocks;
	Area& m_area;
	const Faction& m_faction;
	bool m_enabled;
	HasScheduledEvent<ReenableStockPileScheduledEvent> m_reenableScheduledEvent;
	StockPileProject* m_projectNeedingMoreWorkers;
public:
	StockPile(std::vector<ItemQuery>& q, Area& a, const Faction& f);
	StockPile(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	Json toJson() const;
	bool accepts(const Item& item) const;
	void addBlock(Block& block);
	void removeBlock(Block& block);
	void updateQueries(std::vector<ItemQuery>& queries);
	void incrementOpenBlocks();
	void decrementOpenBlocks();
	void disableIndefinately() { m_enabled = false; }
	void disableTemporarily(Step duration) { m_reenableScheduledEvent.schedule(*this, duration);  disableIndefinately(); }
	void reenable() { m_enabled = true; } 
	void addToProjectNeedingMoreWorkers(Actor& actor, StockPileObjective& objective);
	void destroy();
	[[nodiscard]] bool isEnabled() const { return m_enabled; }
	[[nodiscard]] bool hasProjectNeedingMoreWorkers() const { return m_projectNeedingMoreWorkers != nullptr; }
	[[nodiscard]] Simulation& getSimulation();
	friend class AreaHasStockPilesForFaction;
	friend class ReenableStockPileScheduledEvent;
	friend class StockPileProject;
	friend class BlockIsPartOfStockPiles;
	[[nodiscard]] bool operator==(const StockPile& other) const { return &other == this; }
};
class StockPileProject final : public Project
{
	Item& m_item;
	Item* m_delivered;
	StockPile& m_stockpile;
	bool m_dispatched;
	Step getDuration() const;
	void onComplete();
	void onReserve();
	void onDelivered(HasShape& delivered) { assert(delivered.isItem()); m_delivered = &static_cast<Item&>(delivered); }
	// Mark dispatched as true and dismiss any unassigned workers and candidates.
	void onSubprojectCreated(HaulSubproject& subproject);
	void onCancel();
	// TODO: geometric progresson of disable duration.
	void onDelay() { m_item.m_canBeStockPiled.maybeUnsetAndScheduleReset(m_faction, Config::stepsToDisableStockPile); cancel(); }
	void offDelay() { assert(false); }
	void onHasShapeReservationDishonored(const HasShape& hasShape, uint32_t oldCount, uint32_t newCount);
	[[nodiscard]] bool canReset() const { return false; }
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
public:
	StockPileProject(const Faction* faction, Block& block, Item& item);
	StockPileProject(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	bool canAddWorker(const Actor& actor) const;
	friend class AreaHasStockPilesForFaction;
};
class ReenableStockPileScheduledEvent final : public ScheduledEvent
{
	StockPile& m_stockPile;
public:
	ReenableStockPileScheduledEvent(StockPile& sp, Step duration, const Step start = 0) : ScheduledEvent(sp.getSimulation(), duration, start), m_stockPile(sp) { }
	void execute() { m_stockPile.reenable(); }
	void clearReferences() { m_stockPile.m_reenableScheduledEvent.clearPointer(); }
};
struct BlockIsPartOfStockPile
{
	StockPile& stockPile;
	bool active;
};
class BlockIsPartOfStockPiles
{
	Block& m_block;
	std::unordered_map<const Faction*, BlockIsPartOfStockPile> m_stockPiles;
public:
	BlockIsPartOfStockPiles(Block& b): m_block(b) { }
	void recordMembership(StockPile& stockPile);
	void recordNoLongerMember(StockPile& stockPile);
	// When an item is added or removed update avalibility for all stockpiles.
	void updateActive();
	[[nodiscard]] StockPile* getForFaction(const Faction& faction) { if(!m_stockPiles.contains(&faction)) return nullptr; return &m_stockPiles.at(&faction).stockPile; }
	[[nodiscard]] bool contains(const Faction& faction) const { return m_stockPiles.contains(const_cast<Faction*>(&faction)); }
	[[nodiscard]] bool isAvalible(const Faction& faction) const;
	friend class AreaHasStockPilesForFaction;
};
struct StockPileHasShapeDishonorCallback final : public DishonorCallback
{
	StockPileProject& m_stockPileProject;
	StockPileHasShapeDishonorCallback(StockPileProject& hs) : m_stockPileProject(hs) { } 
	StockPileHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
		m_stockPileProject(static_cast<StockPileProject&>(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
	Json toJson() const { return Json({{"type", "StockPileHasShapeDishonorCallback"}, {"project", reinterpret_cast<uintptr_t>(&m_stockPileProject)}}); }
	// Craft step project cannot reset so cancel instead and allow to be recreated later.
	// TODO: Why?
	void execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount) { m_stockPileProject.cancel(); }
};
class AreaHasStockPilesForFaction
{
	Area& m_area;
	std::list<StockPile> m_stockPiles;
	const Faction& m_faction;
	// Stockpiles may accept multiple item types and thus may appear here more then once.
	std::unordered_map<const ItemType*, std::unordered_set<StockPile*>> m_availableStockPilesByItemType;
	// These items are checked whenever a new stockpile is created to see if they should be move to items with destinations.
	std::unordered_map<const ItemType*, std::unordered_set<Item*>> m_itemsWithoutDestinationsByItemType;
	// Only when an item is added here does it get designated for stockpileing.
	std::unordered_set<Item*> m_itemsWithDestinationsWithoutProjects;
	// The stockpile used as index here is not neccesarily where the item will go, it is used to prove that there is somewhere the item could go.
	std::unordered_map<StockPile*, std::unordered_set<Item*>> m_itemsWithDestinationsByStockPile;
	// Multiple projects per item due to generic item stacking.
	std::unordered_map<Item*, std::list<StockPileProject>> m_projectsByItem;
	// To be called when the last block is removed from the stockpile.
	// To remove all blocks call StockPile::destroy.
	void destroyStockPile(StockPile& stockPile);
public:
	AreaHasStockPilesForFaction(Area& a, const Faction& f) : m_area(a), m_faction(f) { }
	AreaHasStockPilesForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const Faction& f);
	Json toJson() const;
	StockPile& addStockPile(std::vector<ItemQuery>&& queries);
	StockPile& addStockPile(std::vector<ItemQuery>& queries);
	bool isValidStockPileDestinationFor(const Block& block, const Item& item) const;
	void addItem(Item& item);
	void removeItem(Item& item);
	void removeBlock(Block& block);
	void setAvailable(StockPile& stockPile);
	void setUnavailable(StockPile& stockPile);
	void makeProject(Item& item, Block& destination, StockPileObjective& objective);
	void cancelProject(StockPileProject& project);
	void destroyProject(StockPileProject& project);
	[[nodiscard]] bool isAnyHaulingAvalableFor(const Actor& actor) const;
	Item* getHaulableItemForAt(const Actor& actor, Block& block);
	StockPile* getStockPileFor(const Item& item) const;
	friend class StockPileThreadedTask;
	friend class StockPile;
	// For testing.
	[[maybe_unused, nodiscard]] std::unordered_set<Item*>& getItemsWithDestinations() { return m_itemsWithDestinationsWithoutProjects; }
	[[maybe_unused, nodiscard]] std::unordered_map<StockPile*, std::unordered_set<Item*>>& getItemsWithDestinationsByStockPile() { return m_itemsWithDestinationsByStockPile; }
	[[maybe_unused, nodiscard]] uint32_t getItemsWithProjectsCount() { return m_projectsByItem.size(); }
};
class AreaHasStockPiles
{
	Area& m_area;
	std::unordered_map<const Faction*, AreaHasStockPilesForFaction> m_data;
public:
	AreaHasStockPiles(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(const Faction& faction) { assert(!m_data.contains(&faction)); m_data.try_emplace(&faction, m_area, faction); }
	void removeFaction(const Faction& faction) { assert(m_data.contains(&faction)); m_data.erase(&faction); }
	void removeItemFromAllFactions(Item& item) { for(auto& pair : m_data) { pair.second.removeItem(item); } }
	void removeBlockFromAllFactions(Block& block) { for(auto& pair : m_data) { pair.second.removeBlock(block); }} 
	AreaHasStockPilesForFaction& at(const Faction& faction) { assert(m_data.contains(&faction)); return m_data.at(&faction); }
	[[nodiscard]] bool contains(const Faction& faction) { return m_data.contains(&faction); }
};
