#include "stockpile.h"
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
void StockPileThreadedTask::readStep()
{
	auto itemCondition = [&](Block* block)
	{
		return m_objective.m_actor.m_location->m_area->m_hasStockPiles.getHaulableItemForAt(m_objective.m_actor, *block) != nullptr;
	};
	Block* location = path::getForActorToPredicateReturnEndOnly(m_objective.m_actor, itemCondition);
	if(location == nullptr)
		return;
	m_item = m_objective.m_actor.m_location->m_area->m_hasStockPiles.getHaulableItemForAt(m_objective.m_actor, *m_location);
	assert(m_item != nullptr);
	auto destinationCondition = [&](Block* block)
	{
		return block->m_isPartOfStockPile.m_isAvalable && block->m_isPartOfStockPile.accepts(m_item);
	};
	m_destination = path::getForActorFromToPredicateReturnEndOnly(m_objective.m_actor, location, destinationCondition);
	assert(m_destination != nullptr);
}
void StockPileThreadedTask::writeStep()
{
	if(m_item == nullptr)
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
		m_objective.m_actor.m_location->m_area->m_hasStockPiles.makeProject(m_item, m_destination, m_objective.m_actor);
}
bool StockPileObjectiveType::canBeAssigned(Actor& actor)
{
	return actor.m_location->m_area->m_hasStockPiles.isAnyHaulingAvalableFor(actor);
}
std::unique_ptr<ObjectiveType> StockPileObjectiveType::makeFor(Actor& actor)
{
	return std::make_unique<StockPileObjective>(actor);
}
void StockPileObjective::execute() 
{ 
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr);
	m_threadedTask.create(*this); 
	else
		m_project.commandWorker(m_actor);
}
uint32_t StockPileProject::getDelay() const { return Config::addToStockPileDelaySteps; }
void StockPileProject::onComplete()
{
	assert(m_workers.size() == 1);
	m_workers.begin()->m_canPickup.putDown(m_destination);
}
std::vector<std::pair<ItemQuery, uint32_t> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t> StockPileProject::getUnconsumed() const { return {m_item, 1}; }
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
	m_blocks.remove(block);
}
void StockPile::incrementOpenBlocks()
{
	++m_openBlocks;
	if(m_openBlocks == 1)
		m_hasStockPiles.setAvalable(*this);
}
void StockPile::decrementOpenBlocks()
{
	--m_openBlocks;
	if(m_openBlocks == 0)
		m_hasStockPiles.setUnavalable(*this);
}
void BlockIsPartOfStockPile::setStockPile(StockPile& stockPile)
{
	assert(m_stockPile == nullptr);
	m_stockPile = stockPile;
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
	return m_stockPile;
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
	StockPile& stockPile = m_stockPiles.emplace_back(std::move(queries));
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_stockPilesByItemType[itemQuery.itemType].insert(&stockPile);
}
void HasStockPiles::removeStockPile(StockPile& stockPile)
{
	assert(stockPile.m_blocks.empty());
	for(ItemQuery& itemQuery : stockPiles.m_queries)
		m_stockPilesByItemType[itemQuery.itemType].remove(&stockPile);
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
	return accepts(block.m_isPartOfStockPile.getStockPile, item);
}
void HasStockPiles::addItem(Item& item)
{
	StockPile* stockPile = getStockPileFor(item);
	if(stockPile == nullptr)
		m_itemsWithoutDestinations.insert(&item);
	else
	{
		m_itemsWithDestinationsByStockPile[stockPile].insert(&item);
		m_stockPilesByItemWithDestination[&item] = stockPile;
	}
}
void HasStockPiles::removeItem(Item& item)
{
	if(m_itemsWithoutDestinations.contains(&item))
		m_itemsWithoutDestinations.remove(&item);
	else
	{
		assert(m_stockPilesByItemWithDestination.contains(&item));
		m_itemsWithDestinationsByStockPile[m_stockPilesByItemWithDestination.at(&item)].remove(item);
		m_stockPilesByItemWithDestination.remove(&item);
		m_projectsByItem.remove(&item);
	}
}
void HasStockPiles::setAvalable(StockPile& stockPile)
{
	assert(!m_itemsWithDestinationsByStockPile.contains(stockPile);
			for(ItemQuery& itemQuery : stockPile.m_queries)
			{
			assert(itemQuery.itemType != nullptr);
			m_avalableStockPilesByItemType[itemQuery.itemType].insert(&stockPile);
			if(m_itemsWithoutDestinationsByItemType.contains(itemQuery.itemType))
			for(Item* item : m_itemsWithoutDestinationsByItemType.at(itemQuery.itemType))
			if(stockPile.accepts(*item))
			{
			m_stockPilesByItemWithDestination[&item] = stockPile;
			m_itemsWithDestinationsByStockPile[&stockPile].insert(item);
			//TODO: remove item from items without destinations by item type.
			}
			}
			}
			void HasStockPiles::setUnavalable(StockPile& stockPile)
			{

			for(ItemQuery& itemQuery : stockPile.m_queries)
			m_avalableStockPilesByItemType.remove(&stockPile);
			for(Item& item : m_itemsWithDestinationsByStockPile.at(stockPile))
			{
				m_stockPilesByItemWithDestination.remove(&item);
				StockPile* newStockPile = getStockPileFor(item);
				if(newStockPile == nullptr)
				{
					m_itemsWithoutDestinationsByItemType.insert(&item);
					m_projectsByItem.remove(&item);
				}
				else
					m_itemsWithDestinationsByStockPile[newStockPile].insert(&item);
			}
			m_itemsWithDestinationsByStockPile.remove(&stockPile);
			}
void HasStockPiles::makeProject(Item& item, Block& destination, Actor& actor)
{
	assert(!m_projectsByItem.contains(item));
	assert(!destination.m_isPartOfStockPile.empty());
	assert(destination.m_isPartOfStockPile.getIsAvalable());
	m_projectsByItem[item] = std::make_unique<StockPileProject>(destination, item);
	m_projectsByItem[item]->addWorker(actor);	
}
void HasStockPiles::cancelProject(StockPileProject& project)
{
	m_projectsByItem.remove(project.item);
}
bool HasStockPiles::isAnyHaulingAvalableFor(Actor& actor) const
{
	for(auto& pair : m_itemsWithDestinationsByStockPile)
		for(Item* item : pair.second)
			if(actor.m_hasItems.canPickupAny(item))
				return true;
	return false;
}
Item* HasStockPiles::getHaulableItemForAt(Actor& actor, Block& block)
{
	if(block.m_reservable.isFullyReserved())
		return nullptr;
	for(Item& item : block.m_hasItems.get())
		if(!item.m_reservable.isFullyReserved() && actor.m_hasItems.canPickupAny(item) && m_itemsWithDestinations.contains(block))
			return &item;
	return nullptr;
}
StockPile* HasStockPiles::getStockPileFor(Item& item) const
{
	if(!m_stockPilesByItemType.contains(item.itemType))
		return nullptr;
	for(StockPile* stockPile : m_avalableStockPilesByItemType.at(item.itemType))
		if(accepts(*stockPile, item))
			return stockPile;
	return nullptr;
}
