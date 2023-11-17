#include "stockpile.h"
#include "block.h"
#include "area.h"
#include "config.h"
#include "simulation.h"
#include <cwchar>
#include <functional>
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
void StockPileThreadedTask::readStep()
{
	assert(m_objective.m_project == nullptr);
	assert(m_objective.m_actor.m_project == nullptr);
	if(m_item == nullptr)
	{
		std::function<Item*(const Block&)> getItem = [&](const Block& block)
		{
			return m_objective.m_actor.m_location->m_area->m_hasStockPiles.at(*m_objective.m_actor.getFaction()).getHaulableItemForAt(m_objective.m_actor, const_cast<Block&>(block));
		};
		std::function<bool(const Block&)> blocksContainsItemCondition = [&](const Block& block)
		{
			return getItem(block) != nullptr;
		};
		std::function<bool(const Block&)> destinationCondition = [&](const Block& block)
		{
			//TODO: Allow any facing.
			if(!block.m_hasShapes.canEnterCurrentlyWithFacing(*m_item, 0))
				return false;
			if(!block.m_hasItems.empty())
				if(!m_item->isGeneric() || block.m_hasItems.getCount(m_item->m_itemType, m_item->m_materialType) == 0)
					return false;
			const StockPile* stockpile = const_cast<Block&>(block).m_isPartOfStockPiles.getForFaction(*m_objective.m_actor.getFaction());
			const Faction& faction = *m_objective.m_actor.getFaction();
			if(stockpile == nullptr || !stockpile->isEnabled() || !block.m_isPartOfStockPiles.isAvalible(faction))
			       return false;
			if(block.m_reservable.isFullyReserved(&faction))
			       return false;
			if(!stockpile->accepts(*m_item))
				return false;
			return true;
		};
		m_findsPath.pathToUnreservedAdjacentToPredicate(blocksContainsItemCondition, *m_objective.m_actor.getFaction());
		if(m_findsPath.found())
		{
			m_item = getItem(*m_findsPath.getBlockWhichPassedPredicate());
			// Item and path to it found, now check for path to destinaiton.
			// TODO: Path from item location rather then from actor location, requires adding a setStart method to FindsPath.
			FindsPath findAnotherPath(m_objective.m_actor, false);
			findAnotherPath.pathToUnreservedAdjacentToPredicate(destinationCondition, *m_objective.m_actor.getFaction());
			if(findAnotherPath.found())
				m_destination = findAnotherPath.getBlockWhichPassedPredicate();
		}
	}
	else
	{
		assert(m_destination != nullptr);
		if(m_objective.m_actor.m_canPickup.isCarrying(*m_item))
			m_findsPath.pathAdjacentToBlock(*m_destination);
		else
			m_findsPath.pathToUnreservedAdjacentToHasShape(*m_item, *m_objective.m_actor.getFaction());
	}
}
void StockPileThreadedTask::writeStep()
{
	if(m_item == nullptr)
		m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	else
	{
		const Faction& faction = *m_objective.m_actor.getFaction();
		if(!m_destination->m_isPartOfStockPiles.contains(faction) || 
			!m_destination->m_isPartOfStockPiles.getForFaction(faction)->accepts(*m_item)
		)
		{
			// Conditions have changed since read step, create a new task.
			m_objective.m_threadedTask.create(m_objective);
			return;
		}
		// StockPile candidate found, create a new project or add to an existing one.
		StockPile& stockpile = *m_destination->m_isPartOfStockPiles.getForFaction(faction);
		if(stockpile.hasProjectNeedingMoreWorkers())
			stockpile.addToProjectNeedingMoreWorkers(m_objective.m_actor, m_objective);
		else
		{
			auto& hasStockPiles = m_destination->m_area->m_hasStockPiles.at(faction);
			if(hasStockPiles.m_projectsByItem.contains(m_item))
			{
				// Projects found, select one to join.
				for(StockPileProject& stockPileProject : hasStockPiles.m_projectsByItem.at(m_item))
					if(stockPileProject.canAddWorker(m_objective.m_actor))
					{
						m_objective.m_project = &stockPileProject;
						stockPileProject.addWorkerCandidate(m_objective.m_actor, m_objective);
					}
			}
			else
				// No projects found, make one.
				hasStockPiles.makeProject(*m_item, *m_destination, m_objective);
		}
	}
}
void StockPileThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
bool StockPileObjectiveType::canBeAssigned(Actor& actor) const
{
	return actor.m_location->m_area->m_hasStockPiles.at(*actor.getFaction()).isAnyHaulingAvalableFor(actor);
}
std::unique_ptr<Objective> StockPileObjectiveType::makeFor(Actor& actor) const
{
	return std::make_unique<StockPileObjective>(actor);
}
void StockPileObjective::execute() 
{ 
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
	{
		assert(m_actor.m_project == nullptr);
		m_threadedTask.create(*this); 
	}
	else
		m_project->commandWorker(m_actor);
}
void StockPileObjective::cancel()
{
	if(m_project != nullptr)
	{
		m_project->removeWorker(m_actor);
		if(m_project->getWorkers().empty())
			m_project->cancel();
	}
	m_threadedTask.maybeCancel();
}
void StockPileObjective::reset()
{
	cancel();
	m_project = nullptr;
	m_actor.m_project = nullptr;
	m_actor.m_canReserve.clearAll();
}
StockPileProject::StockPileProject(const Faction* faction, Block& block, Item& item) : Project(faction, block, Config::maxWorkersForStockPileProject), m_item(item), m_delivered(nullptr), m_stockpile(*block.m_isPartOfStockPiles.getForFaction(*faction)), m_dispatched(false) { }
bool StockPileProject::canAddWorker(const Actor& actor) const
{
	if(m_dispatched)
		return false;
	return Project::canAddWorker(actor);
}
Step StockPileProject::getDuration() const { return Config::addToStockPileDelaySteps; }
void StockPileProject::onComplete()
{
	assert(m_delivered != nullptr);
	m_delivered->setLocation(m_location);
	auto workers = std::move(m_workers);
	m_location.m_area->m_hasStockPiles.at(m_stockpile.m_faction).removeItem(m_item);
	for(auto& [actor, projectWorker] : workers)
		actor->m_hasObjectives.objectiveComplete(projectWorker.objective);
}
void StockPileProject::onReserve()
{
	// Project has reserved it's item and destination, as well as haul tool and / or beast of burden.
	// Clear from StockPile::m_projectNeedingMoreWorkers.
	if(m_stockpile.m_projectNeedingMoreWorkers == this)
		m_stockpile.m_projectNeedingMoreWorkers = nullptr;
}
void StockPileProject::onSubprojectCreated(HaulSubproject& subproject)
{
	m_dispatched = true;
	if(m_item.m_reservable.getUnreservedCount(m_stockpile.m_faction) == 0)
		m_item.m_canBeStockPiled.unset(m_stockpile.m_faction);
	std::vector<std::pair<Actor*, Objective*>> toDismiss;
	for(auto& pair : m_workers)
		if(pair.second.haulSubproject == nullptr)
			toDismiss.emplace_back(pair.first, &pair.second.objective);
		else
			assert(pair.second.haulSubproject == &subproject);
	for(auto& pair : m_workerCandidatesAndTheirObjectives)
		toDismiss.push_back(pair);
	for(auto& [actor, objective] : toDismiss)
	{
		objective->reset();
		objective->execute();
	}
}
void StockPileProject::onCancel()
{
	m_item.m_canBeStockPiled.set(m_stockpile.m_faction);
}
std::vector<std::pair<ItemQuery, uint32_t>> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> StockPileProject::getUnconsumed() const { return {{m_item, 1}}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> StockPileProject::getByproducts() const {return {}; }
std::vector<std::pair<ActorQuery, uint32_t>> StockPileProject::getActors() const { return {}; }
bool StockPile::accepts(const Item& item) const 
{
	for(const ItemQuery& itemQuery : m_queries)
		if(itemQuery(item))
			return true;
	return false;
}
void StockPile::addBlock(Block& block)
{
	assert(!m_blocks.contains(&block));
	assert(!block.m_isPartOfStockPiles.contains(m_faction));
	block.m_isPartOfStockPiles.recordMembership(*this);
	if(block.m_isPartOfStockPiles.isAvalible(m_faction))
		incrementOpenBlocks();
	m_blocks.insert(&block);
}
StockPile::StockPile(std::vector<ItemQuery>& q, Area& a, const Faction& f) : m_queries(q), m_openBlocks(0), m_area(a), m_faction(f), m_enabled(true), m_reenableScheduledEvent(m_area.m_simulation.m_eventSchedule), m_projectNeedingMoreWorkers(nullptr) { }
void StockPile::removeBlock(Block& block)
{
	assert(m_blocks.contains(&block));
	assert(block.m_isPartOfStockPiles.contains(m_faction));
	assert(block.m_isPartOfStockPiles.getForFaction(m_faction) == this);
	block.m_isPartOfStockPiles.recordNoLongerMember(*this);
	if(block.m_isPartOfStockPiles.isAvalible(m_faction))
		decrementOpenBlocks();
	m_blocks.erase(&block);
}
void StockPile::incrementOpenBlocks()
{
	++m_openBlocks;
	if(m_openBlocks == 1)
		m_area.m_hasStockPiles.at(m_faction).setAvailable(*this);
}
void StockPile::decrementOpenBlocks()
{
	--m_openBlocks;
	if(m_openBlocks == 0)
		m_area.m_hasStockPiles.at(m_faction).setUnavailable(*this);
}
Simulation& StockPile::getSimulation()
{
	assert(!m_blocks.empty());
	return (*m_blocks.begin())->m_area->m_simulation;
}
void StockPile::addToProjectNeedingMoreWorkers(Actor& actor, StockPileObjective& objective)
{
	assert(m_projectNeedingMoreWorkers != nullptr);
	m_projectNeedingMoreWorkers->addWorkerCandidate(actor, objective);
}
void BlockIsPartOfStockPiles::recordMembership(StockPile& stockPile)
{
	assert(!contains(stockPile.m_faction));
	m_stockPiles.try_emplace(&stockPile.m_faction, stockPile, false);
	if(isAvalible(stockPile.m_faction))
	{
		m_stockPiles.at(&stockPile.m_faction).active = true;
		stockPile.incrementOpenBlocks();
	}
}
void BlockIsPartOfStockPiles::recordNoLongerMember(StockPile& stockPile)
{
	assert(contains(stockPile.m_faction));
	if(isAvalible(stockPile.m_faction))
		stockPile.decrementOpenBlocks();
	m_stockPiles.erase(&stockPile.m_faction);
}
void BlockIsPartOfStockPiles::updateActive()
{
	for(auto& [faction, blockHasStockPile] : m_stockPiles)
	{
		bool avalible = isAvalible(*faction);
		if(avalible && !blockHasStockPile.active)
		{
			blockHasStockPile.active = true;
			blockHasStockPile.stockPile.incrementOpenBlocks();
		}
		else if(!avalible && blockHasStockPile.active)
		{
			blockHasStockPile.active = false;
			blockHasStockPile.stockPile.decrementOpenBlocks();
		}
	}
}
bool BlockIsPartOfStockPiles::isAvalible(const Faction& faction) const 
{ 
	if(m_block.m_hasItems.empty())
		return true;
	if(m_block.m_hasItems.getAll().size() > 1)
		return false;
	Item& item = *m_block.m_hasItems.getAll().front();
	if(!item.isGeneric())
		return false;
	StockPile& stockPile = m_stockPiles.at(&faction).stockPile;
	if(!stockPile.accepts(item))
		return false;
	Volume volume = item.m_itemType.volume;
	return m_block.m_hasShapes.getStaticVolume() + volume <= Config::maxBlockVolume;
}
StockPile& AreaHasStockPilesForFaction::addStockPile(std::vector<ItemQuery>&& queries) { return addStockPile(queries); }
StockPile& AreaHasStockPilesForFaction::addStockPile(std::vector<ItemQuery>& queries)
{
	StockPile& stockPile = m_stockPiles.emplace_back(queries, m_area, m_faction);
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].insert(&stockPile);
	return stockPile;
}
void AreaHasStockPilesForFaction::removeStockPile(StockPile& stockPile)
{
	assert(stockPile.m_blocks.empty());
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].erase(&stockPile);
	m_stockPiles.remove(stockPile);
}
bool AreaHasStockPilesForFaction::isValidStockPileDestinationFor(const Block& block, const Item& item) const
{
	if(block.m_reservable.isFullyReserved(&m_faction))
		return false;
	if(!block.m_isPartOfStockPiles.isAvalible(m_faction))
		return false;
	return const_cast<BlockIsPartOfStockPiles&>(block.m_isPartOfStockPiles).getForFaction(m_faction)->accepts(item);
}
void AreaHasStockPilesForFaction::addItem(Item& item)
{
	item.m_canBeStockPiled.set(m_faction);
	// This isn't neccessarily the stockpile that the item is going to, it is only the one that proved that there exists a stockpile somewhere which accepts this item.
	StockPile* stockPile = getStockPileFor(item);
	if(stockPile == nullptr)
		m_itemsWithoutDestinationsByItemType[&item.m_itemType].insert(&item);
	else
	{
		m_itemsWithDestinationsByStockPile[stockPile].insert(&item);
		m_itemsWithDestinationsWithoutProjects.insert(&item);
	}
}
void AreaHasStockPilesForFaction::removeItem(Item& item)
{
	item.m_canBeStockPiled.maybeUnset(m_faction);
	if(m_itemsWithoutDestinationsByItemType.contains(&item.m_itemType) && m_itemsWithoutDestinationsByItemType.at(&item.m_itemType).contains(&item))
		m_itemsWithoutDestinationsByItemType.at(&item.m_itemType).erase(&item);
	else
	{
		m_itemsWithDestinationsWithoutProjects.erase(&item);
		if(m_projectsByItem.contains(&item))
		{
			for(StockPileProject& stockPileProject : m_projectsByItem.at(&item))
			{
				StockPile& stockPile = stockPileProject.m_stockpile;
				if(m_itemsWithDestinationsByStockPile.at(&stockPile).size() == 1)
					m_itemsWithDestinationsByStockPile.erase(&stockPile);
				else
					m_itemsWithDestinationsByStockPile.at(&stockPile).erase(&item);
			}
			m_projectsByItem.erase(&item);
		}
	}
}
void AreaHasStockPilesForFaction::setAvailable(StockPile& stockPile)
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
void AreaHasStockPilesForFaction::setUnavailable(StockPile& stockPile)
{
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType.at(itemQuery.m_itemType).erase(&stockPile);
	for(Item* item : m_itemsWithDestinationsByStockPile.at(&stockPile))
	{
		m_projectsByItem.erase(item);
		StockPile* newStockPile = getStockPileFor(*item);
		if(newStockPile == nullptr)
			m_itemsWithoutDestinationsByItemType.at(&item->m_itemType).insert(item);
		else
			m_itemsWithDestinationsByStockPile[newStockPile].insert(item);
	}
	m_itemsWithDestinationsByStockPile.erase(&stockPile);
}
void AreaHasStockPilesForFaction::makeProject(Item& item, Block& destination, StockPileObjective& objective)
{
	assert(destination.m_isPartOfStockPiles.contains(*objective.m_actor.getFaction()));
	StockPileProject& project = m_projectsByItem[&item].emplace_back(objective.m_actor.getFaction(), destination, item);
	project.addWorkerCandidate(objective.m_actor, objective);	
	objective.m_project = &project;
}
void AreaHasStockPilesForFaction::cancelProject(StockPileProject& project)
{
	Item& item = project.m_item;
	m_projectsByItem.at(&project.m_item).remove(project);
	addItem(item);
}
bool AreaHasStockPilesForFaction::isAnyHaulingAvalableFor(const Actor& actor) const
{
	assert(&m_faction == actor.getFaction());
	return !m_itemsWithDestinationsByStockPile.empty();
}
Item* AreaHasStockPilesForFaction::getHaulableItemForAt(const Actor& actor, Block& block)
{
	const Faction& faction = *actor.getFaction();
	if(block.m_reservable.isFullyReserved(&faction))
		return nullptr;
	for(Item* item : block.m_hasItems.getAll())
	{
		if(item->m_reservable.isFullyReserved(&faction))
			continue;
		if(item->m_canBeStockPiled.contains(faction))
			return item;
	}
	return nullptr;
}
StockPile* AreaHasStockPilesForFaction::getStockPileFor(const Item& item) const
{
	if(!m_availableStockPilesByItemType.contains(&item.m_itemType))
		return nullptr;
	for(StockPile* stockPile : m_availableStockPilesByItemType.at(&item.m_itemType))
		if(stockPile->accepts(item))
			return stockPile;
	return nullptr;
}
