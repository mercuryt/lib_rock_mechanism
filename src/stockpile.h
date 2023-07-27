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
	StockPileObjective(uint32_t priority, Actor& a) : Objective(priority), m_actor(a) { }
	void execute();
	void cancel() { }
};
class StockPileProject final : public Project
{
	Item& m_item;
	StockPileProject(Block& block, Item& item) : Project(block, 1), m_item(item) { }
public:
	uint32_t getDelay() const;
	void onComplete();
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
};
class StockPile
{
	std::vector<ItemQuery> m_queries;
	std::unordered_set<Block*> m_blocks;
	uint32_t m_openBlocks;
	Area& m_area;
public:
	StockPile(std::vector<ItemQuery>& q, Area& a) : m_queries(q), m_openBlocks(0), m_area(a) { }
	bool accepts(Item& item);
	void addBlock(Block* block);
	void removeBlock(Block* block);
	void incrementOpenBlocks();
	void decrementOpenBlocks();
	friend class HasStockPiles;
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
	std::unordered_map<const ItemType*, std::unordered_set<StockPile*>> m_availableStockPilesByItemType;
	std::unordered_set<const ItemType*, Item*> m_itemsWithoutDestinationsByItemType;
	std::unordered_set<Item*> m_itemsWithDestinations;
	std::unordered_map<StockPile*, std::unordered_set<Item*>> m_itemsWithDestinationsByStockPile;
	std::unordered_map<Item*, StockPileProject> m_projectsByItem;
public:
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
