#pragma once
#include "objective.h"
#include "threadedTask.h"
#include "project.h"
#include "item.h"
#include "actor.h"
#include "project.h"

#include <memory>
#include <vector>
#include <utility>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <list>

class Block;
class StockPileObjective;
class StockPileProject;
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
class StockPileThreadedTask final : public ThreadedTask
{
	StockPileObjective& m_objective;
	Item* m_item;
	Block* m_destination;
public:
	StockPileThreadedTask(StockPileObjective& spo) : m_objective(spo), m_item(nullptr), m_destination(nullptr) { }
	void readStep();
	void writeStep();
	void clearReferences();
};
class StockPileObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
};
class StockPileObjective final : public Objective
{
public:
	Actor& m_actor;
	HasThreadedTask<StockPileThreadedTask> m_threadedTask;
	StockPileProject* m_project;
	StockPileObjective(Actor& a) : Objective(Config::stockPilePriority), m_actor(a) { }
	void execute();
	void cancel() { }
};
class StockPileProject final : public Project
{
	Item& m_item;
public:
	StockPileProject(const Faction& faction, Block& block, Item& item) : Project(faction, block, 1), m_item(item) { }
	Step getDelay() const;
	void onComplete();
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	friend class HasStockPiles;
};
class StockPile
{
	std::vector<ItemQuery> m_queries;
	std::unordered_set<Block*> m_blocks;
	uint32_t m_openBlocks;
	Area& m_area;
	const Faction& m_faction;
public:
	StockPile(std::vector<ItemQuery>& q, Area& a, const Faction& f) : m_queries(q), m_openBlocks(0), m_area(a), m_faction(f) { }
	bool accepts(const Item& item) const;
	void addBlock(Block* block);
	void removeBlock(Block* block);
	void incrementOpenBlocks();
	void decrementOpenBlocks();
	friend class HasStockPiles;
	bool operator==(const StockPile& other) const { return &other == this; }
};
class BlockIsPartOfStockPile
{
	Block& m_block;
	StockPile* m_stockPile;
	bool m_isAvalable;
public:
	BlockIsPartOfStockPile(Block& b) : m_block(b), m_stockPile(nullptr), m_isAvalable(true) { }
	void setStockPile(StockPile& stockPile);
	void clearStockPile();
	StockPile& getStockPile();
	const StockPile& getStockPile() const;
	void setAvalable();
	void setUnavalable();
	bool hasStockPile() const;
	bool getIsAvalable(const Faction& faction) const;
	friend class StockPileThreadedTask;
};
class HasStockPiles
{
	Area& m_area;
	std::list<StockPile> m_stockPiles;
	std::unordered_map<const ItemType*, std::unordered_set<StockPile*>> m_availableStockPilesByItemType;
	std::unordered_map<const ItemType*, std::unordered_set<Item*>> m_itemsWithoutDestinationsByItemType;
	std::unordered_set<Item*> m_itemsWithDestinations;
	std::unordered_map<StockPile*, std::unordered_set<Item*>> m_itemsWithDestinationsByStockPile;
	std::unordered_map<Item*, StockPileProject> m_projectsByItem;
public:
	HasStockPiles(Area& a) : m_area(a) { }
	void addStockPile(std::vector<ItemQuery>&& queries, const Faction& faction);
	void removeStockPile(StockPile& stockPile);
	bool isValidStockPileDestinationFor(const Block& block, const Item& item, const Faction& faction) const;
	void addItem(Item& item);
	void removeItem(Item& item);
	void setAvalable(StockPile& stockPile);
	void setUnavalable(StockPile& stockPile);
	void makeProject(Item& item, Block& destination, Actor& actor);
	void cancelProject(StockPileProject& project);
	bool isAnyHaulingAvalableFor(const Actor& actor) const;
	Item* getHaulableItemForAt(const Actor& actor, Block& block);
	StockPile* getStockPileFor(const Item& item) const;
	friend class StockPileThreadedTask;
};
