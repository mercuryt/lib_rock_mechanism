#pragma once
#include "objective.h"
#include "threadedTask.h"
#include "project.h"
#include "item.h"
#include "block.h"
#include "actor.h"

#include <memory>
#include <vector>
#include <utility>
#include <tuple>
#include <unordered_set>
#include <unordered_map>
#include <list>

// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
class StockPileThreadedTask : ThreadedTask
{
	StockPileObjective& m_objective;
	Item* m_item;
	Block* m_destination;
public:
	StockPileThreadedTask(StockPileObjective& spo) : m_objective(spo), m_block(nullptr), m_item(nullptr) { }
	void readStep();
	void writeStep();
};
class StockPileObjectiveType : ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor);
	std::unique_ptr<ObjectiveType> makeFor(Actor& actor);
};
class StockPileObjective : Objective
{
public:
	Actor& m_actor;
	HasThreadedTask<StockPileThreadedTask> m_threadedTask;
	StockPileProject* m_project;
	StockPileObjective(uint32_t priority, Actor& a) : Objective(priority), m_actor(a) { }
	void execute();
};
class StockPileProject : Project
{
	Item& m_item;
	StockPileProject(Block& block, Item& item) : Project(block, 1), m_item(item) { }
public:
	uint32_t getDelay() const;
	void onComplete();
	std::vector<std::pair<ItemQuery, uint32_t> getConsumed();
	std::vector<std::pair<ItemQuery, uint32_t> getUnconsumed();
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
};
class StockPile
{
	std::vector<ItemQuery> m_queries;
	std::unordered_set<Block*> m_blocks;
	uint32_t m_openBlocks;
public:
	StockPile(std::vector<ItemQuery>& queries) : m_queries(queries), m_openBlocks(0) { }
	bool accepts(Item& item);
	void addBlock(Block* block);
	void removeBlock(Block* block);
	void incrementOpenBlocks();
	void decrementOpenBlocks();
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
	bool hasStockPile() const;
	StockPile& getStockPile();
	void setAvalable();
	void setUnavalable();
	bool getIsAvalable() const;
};
class HasStockPiles
{
	std::list<StockPile> m_stockPiles;
	std::unordered_map<const ItemType*, std::unordered_set<StockPile*>> m_avalableStockPilesByItemType;
	std::unordered_set<const ItemType*, Item*> m_itemsWithoutDestinationsByItemType;
	std::unordered_set<Item*> m_itemsWithDestinations;
	std::unordered_map<StockPile*, std::unordered_set<Item*>> m_itemsWithDestinationsByStockPile;
	std::unordered_map<Item*, StockPileProject> m_projectsByItem;
public:
	HasStockPiles() : m_anyHaulingAvalible(false) { }
	void addStockPile(std::vector<ItemQuery>&& queries);
	void removeStockPile(StockPile& stockPile);
	bool isValidStockPileDestinationFor(Block& block, Item& item) const;
	void addItem(Item& item);
	void removeItem(Item& item);
	void setAvalable(StockPile& stockPile);
	void setUnavalable(StockPile& stockPile);
	void makeProject(Item& item, Block& destination, Actor& actor);
	void cancelProject(StockPileProject& project);
	bool isAnyHaulingAvalableFor(Actor& actor) const;
	Item* getHaulableItemForAt(Actor& actor, Block& block);
	StockPile* getStockPileFor(Item& item) const;
};
