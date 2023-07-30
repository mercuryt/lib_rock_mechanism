#include "stockpile.h"
#include "block.h"
#include "area.h"
#include "path.h"
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
void StockPileThreadedTask::readStep()
{
	auto itemCondition = [&](Block& block)
	{
		return m_objective.m_actor.m_location->m_area->m_hasStockPiles.getHaulableItemForAt(m_objective.m_actor, block) != nullptr;
	};
	Block* location = path::getForActorToPredicateReturnEndOnly(m_objective.m_actor, itemCondition);
	if(location == nullptr)
		return;
	m_item = m_objective.m_actor.m_location->m_area->m_hasStockPiles.getHaulableItemForAt(m_objective.m_actor, *location);
	assert(m_item != nullptr);
	auto destinationCondition = [&](Block& block)
	{
		return block.m_isPartOfStockPile.m_isAvalable && block.m_isPartOfStockPile.getStockPile().accepts(*m_item);
	};
	m_destination = path::getForActorFromLocationToPredicateReturnEndOnly(m_objective.m_actor, *location, destinationCondition);
	assert(m_destination != nullptr);
}
void StockPileThreadedTask::writeStep()
{
	if(m_item == nullptr)
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
		m_objective.m_actor.m_location->m_area->m_hasStockPiles.makeProject(*m_item, *m_destination, m_objective.m_actor);
}
bool StockPileObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasStockPiles.isAnyHaulingAvalableFor(actor);
}
std::unique_ptr<Objective> StockPileObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<StockPileObjective>(actor);
}
void StockPileObjective::execute() 
{ 
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
		m_threadedTask.create(*this); 
	else
		m_project->commandWorker(m_actor);
}
uint32_t StockPileProject::getDelay() const { return Config::addToStockPileDelaySteps; }
void StockPileProject::onComplete()
{
	assert(m_workers.size() == 1);
	m_workers.begin()->first->m_canPickup.putDown(m_location);
}
std::vector<std::pair<ItemQuery, uint32_t>> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> StockPileProject::getUnconsumed() const { return {{m_item, 1}}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> StockPileProject::getByproducts() const {return {}; }
bool StockPile::accepts(Item& item)
{
	for(ItemQuery& itemQuery : m_queries)
		if(itemQuery(item))
			return true;
	return false;
}
void StockPile::addBlock(Block* block)
{
	assert(!m_blocks.contains(block));
	assert(!block->m_isPartOfStockPile.hasStockPile());
	block->m_isPartOfStockPile.setStockPile(*this);
	if(block->m_isPartOfStockPile.getIsAvalable())
		incrementOpenBlocks();
	m_blocks.insert(block);
}
void StockPile::removeBlock(Block* block)
{
	assert(m_blocks.contains(block));
	assert(block->m_isPartOfStockPile.hasStockPile());
	assert(&block->m_isPartOfStockPile.getStockPile() == this);
	block->m_isPartOfStockPile.clearStockPile();
	if(block->m_isPartOfStockPile.getIsAvalable())
		decrementOpenBlocks();
	m_blocks.erase(block);
}
void StockPile::incrementOpenBlocks()
{
	++m_openBlocks;
	if(m_openBlocks == 1)
		m_area.m_hasStockPiles.setAvalable(*this);
}
void StockPile::decrementOpenBlocks()
{
	--m_openBlocks;
	if(m_openBlocks == 0)
		m_area.m_hasStockPiles.setUnavalable(*this);
}
void BlockIsPartOfStockPile::setStockPile(StockPile& stockPile)
{
	assert(m_stockPile == nullptr);
	m_stockPile = &stockPile;
}
void BlockIsPartOfStockPile::clearStockPile()
{
	assert(m_stockPile != nullptr);
	m_stockPile = nullptr;
}
bool BlockIsPartOfStockPile::hasStockPile() const
{
	return m_stockPile != nullptr;
}
StockPile& BlockIsPartOfStockPile::getStockPile()
{
	return *m_stockPile;
}
void BlockIsPartOfStockPile::setAvalable()
{
	assert(!m_isAvalable);
	assert(m_stockPile != nullptr);
	m_isAvalable = true;
	m_stockPile->incrementOpenBlocks();
}
void BlockIsPartOfStockPile::setUnavalable()
{
	assert(m_isAvalable);
	assert(m_stockPile != nullptr);
	m_isAvalable = false;
	m_stockPile->decrementOpenBlocks();
}
bool BlockIsPartOfStockPile::getIsAvalable() const { return !m_block.m_reservable.isFullyReserved() && m_block.m_hasItems.empty(); }
void HasStockPiles::addStockPile(std::vector<ItemQuery>&& queries)
{
	StockPile& stockPile = m_stockPiles.emplace_back(queries, m_area);
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].insert(&stockPile);
}
void HasStockPiles::removeStockPile(StockPile& stockPile)
{
	assert(stockPile.m_blocks.empty());
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].erase(&stockPile);
	m_stockPiles.remove(stockPile);
}
bool HasStockPiles::isValidStockPileDestinationFor(Block& block, Item& item) const
{
	if(block.m_reservable.isFullyReserved())
		return false;
	if(!block.m_hasItems.empty())
		return false;
	if(!block.m_isPartOfStockPile.hasStockPile())
		return false;
	return block.m_isPartOfStockPile.getStockPile().accepts(item);
}
void HasStockPiles::addItem(Item& item)
{
	StockPile* stockPile = getStockPileFor(item);
	if(stockPile == nullptr)
		m_itemsWithoutDestinationsByItemType.at(&item.m_itemType).insert(&item);
	else
	{
		m_itemsWithDestinationsByStockPile[stockPile].insert(&item);
	}
}
void HasStockPiles::removeItem(Item& item)
{
	if(m_itemsWithoutDestinationsByItemType.at(&item.m_itemType).contains(&item))
		m_itemsWithoutDestinationsByItemType.at(&item.m_itemType).erase(&item);
	else
	{
		assert(m_itemsWithDestinations.contains(&item));
		m_itemsWithDestinations.erase(&item);
		assert(m_projectsByItem.contains(&item));
		m_projectsByItem.erase(&item);
	}
}
void HasStockPiles::setAvalable(StockPile& stockPile)
{
	assert(!m_itemsWithDestinationsByStockPile.contains(&stockPile));
	for(ItemQuery& itemQuery : stockPile.m_queries)
	{
		assert(itemQuery.m_itemType != nullptr);
		m_availableStockPilesByItemType[itemQuery.m_itemType].insert(&stockPile);
		if(m_itemsWithoutDestinationsByItemType.contains(itemQuery.m_itemType))
			for(Item* item : m_itemsWithoutDestinationsByItemType.at(itemQuery.m_itemType))
				if(stockPile.accepts(*item))
				{
					m_itemsWithDestinationsByStockPile[&stockPile].insert(item);
					//TODO: remove item from items without destinations by item type.
				}
	}
}
void HasStockPiles::setUnavalable(StockPile& stockPile)
{

	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType.at(itemQuery.m_itemType).erase(&stockPile);
	for(Item* item : m_itemsWithDestinationsByStockPile.at(&stockPile))
	{
		m_projectsByItem.erase(item);
		StockPile* newStockPile = getStockPileFor(*item);
		if(newStockPile == nullptr)
		{
			m_itemsWithoutDestinationsByItemType.at(&item->m_itemType).insert(item);
			m_projectsByItem.erase(item);
		}
		else
			m_itemsWithDestinationsByStockPile[newStockPile].insert(item);
	}
	m_itemsWithDestinationsByStockPile.erase(&stockPile);
}
void HasStockPiles::makeProject(Item& item, Block& destination, Actor& actor)
{
	assert(!m_projectsByItem.contains(&item));
	assert(!destination.m_isPartOfStockPile.hasStockPile());
	assert(destination.m_isPartOfStockPile.getIsAvalable());
	m_projectsByItem.try_emplace(&item, destination, item);
	m_projectsByItem.at(&item).addWorker(actor);	
}
void HasStockPiles::cancelProject(StockPileProject& project)
{
	m_projectsByItem.erase(&project.m_item);
}
bool HasStockPiles::isAnyHaulingAvalableFor(Actor& actor) const
{
	for(auto& pair : m_itemsWithDestinationsByStockPile)
		for(Item* item : pair.second)
			if(actor.m_canPickup.canPickupAny(*item))
				return true;
	return false;
}
Item* HasStockPiles::getHaulableItemForAt(Actor& actor, Block& block)
{
	if(block.m_reservable.isFullyReserved())
		return nullptr;
	for(Item* item : block.m_hasItems.getAll())
		if(!item->m_reservable.isFullyReserved() && actor.m_canPickup.canPickupAny(*item) && m_itemsWithDestinations.contains(item))
			return item;
	return nullptr;
}
StockPile* HasStockPiles::getStockPileFor(Item& item) const
{
	if(!m_availableStockPilesByItemType.contains(&item.m_itemType))
		return nullptr;
	for(StockPile* stockPile : m_availableStockPilesByItemType.at(&item.m_itemType))
		if(stockPile->accepts(item))
			return stockPile;
	return nullptr;
}
