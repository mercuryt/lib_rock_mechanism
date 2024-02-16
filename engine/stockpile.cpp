#include "stockpile.h"
#include "block.h"
#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "item.h"
#include "rain.h"
#include "reservable.h"
#include "simulation.h"
#include "stocks.h"
#include <cwchar>
#include <functional>
#include <memory>
//Input
void StockPileCreateInputAction::execute()
{
	auto& hasStockPiles = (*m_cuboid.begin()).m_area->m_hasStockPiles.at(m_faction);
	StockPile& stockpile = hasStockPiles.addStockPile(m_queries);
	for(Block& block : m_cuboid)
		stockpile.addBlock(block);
}
void StockPileRemoveInputAction::execute()
{
	auto& hasStockPiles = (*m_cuboid.begin()).m_area->m_hasStockPiles.at(m_faction);
	for(Block& block : m_cuboid)
		hasStockPiles.removeBlock(block);
}
void StockPileExpandInputAction::execute()
{
	for(Block& block : m_cuboid)
		m_stockpile.addBlock(block);
}
void StockPileUpdateInputAction::execute()
{
	m_stockpile.updateQueries(m_queries);
}
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
			// This is a nested path condition. Is there a faster way?
			// Find a path to a stockpileable item and then find a path to an appropriate stock pile.
			// Would it be better to collect a list of stockpiles reach and if we can reach them and then check each item against that prior to resorting to repeated nested paths?
			Item* item = getItem(block);
			if(item == nullptr)
				return false;
			// Item and path to it found, now check for path to destinaiton.
			// TODO: Path from item location rather then from actor location, requires adding a setStart method to FindsPath.
			std::function<bool(const Block&)> destinationCondition = [&](const Block& block)
			{
				if(!block.m_hasShapes.canEnterCurrentlyWithAnyFacing(*item))
					return false;
				if(!block.m_hasItems.empty())
					if(!item->isGeneric() || block.m_hasItems.getCount(item->m_itemType, item->m_materialType) == 0)
						return false;
				const StockPile* stockpile = const_cast<Block&>(block).m_isPartOfStockPiles.getForFaction(*m_objective.m_actor.getFaction());
				const Faction& faction = *m_objective.m_actor.getFaction();
				if(stockpile == nullptr || !stockpile->isEnabled() || !block.m_isPartOfStockPiles.isAvalible(faction))
					return false;
				if(block.m_reservable.isFullyReserved(&faction))
					return false;
				if(!stockpile->accepts(*item))
					return false;
				m_destination = &const_cast<Block&>(block);
				return true;
			};
			FindsPath findAnotherPath(m_objective.m_actor, false);
			findAnotherPath.pathToUnreservedAdjacentToPredicate(destinationCondition, *m_objective.m_actor.getFaction());
			return findAnotherPath.found() || findAnotherPath.m_useCurrentLocation;
		};
		m_findsPath.pathToUnreservedAdjacentToPredicate(blocksContainsItemCondition, *m_objective.m_actor.getFaction());
		if(m_findsPath.found())
			m_item = getItem(*m_findsPath.getBlockWhichPassedPredicate());
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
StockPileObjective::StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo),
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine), 
	m_project(data.contains("project") ? static_cast<StockPileProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json StockPileObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(m_project != nullptr)
		data["project"] = *m_project;
	return data;
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
		m_project = nullptr;
		m_actor.m_project = nullptr;
	}
	else
		assert(m_actor.m_project == nullptr);
	m_threadedTask.maybeCancel();
	m_actor.m_canReserve.clearAll();
}
StockPileProject::StockPileProject(const Faction* faction, Block& block, Item& item) : Project(faction, block, Config::maxWorkersForStockPileProject), m_item(item), m_delivered(nullptr), m_stockpile(*block.m_isPartOfStockPiles.getForFaction(*faction)), m_dispatched(false) { }
StockPileProject::StockPileProject(const Json& data, DeserializationMemo& deserializationMemo) : 
	Project(data, deserializationMemo),
	m_item(deserializationMemo.m_simulation.getItemById(data["item"].get<ItemId>())), 
	m_delivered(data.contains("delivered") ? &deserializationMemo.m_simulation.getItemById(data["delivered"].get<ItemId>()) : nullptr), 
	m_stockpile(*deserializationMemo.m_stockpiles.at(data["stockpile"].get<uintptr_t>())),
	m_dispatched(data["dispatched"].get<bool>()) { }
Json StockPileProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item.m_id;
	data["stockpile"] = reinterpret_cast<uintptr_t>(&m_stockpile);
	data["dispatched"] = m_dispatched;
	if(m_delivered)
		data["delivered"] = m_delivered->m_id;
	return data;
}
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
	m_location.m_area->m_hasStocks.at(m_faction).record(*m_delivered);
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
	std::vector<Actor*> actors = getWorkersAndCandidates();
	m_location.m_area->m_hasStockPiles.at(m_faction).destroyProject(*this);
	for(Actor* actor : actors)
	{
		static_cast<StockPileObjective&>(actor->m_hasObjectives.getCurrent()).m_project = nullptr;
		actor->m_project = nullptr;
		actor->m_hasObjectives.getCurrent().reset();
		actor->m_hasObjectives.cannotCompleteTask();
	}
}
void StockPileProject::onHasShapeReservationDishonored([[maybe_unused]] const HasShape& hasShape, [[maybe_unused]] uint32_t oldCount, uint32_t newCount)
{
	if(newCount == 0)
		cancel();
}
std::vector<std::pair<ItemQuery, uint32_t>> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, uint32_t>> StockPileProject::getUnconsumed() const { return {{m_item, 1}}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> StockPileProject::getByproducts() const {return {}; }
std::vector<std::pair<ActorQuery, uint32_t>> StockPileProject::getActors() const { return {}; }
StockPile::StockPile(std::vector<ItemQuery>& q, Area& a, const Faction& f) : m_queries(q), m_openBlocks(0), m_area(a), m_faction(f), m_enabled(true), m_reenableScheduledEvent(m_area.m_simulation.m_eventSchedule), m_projectNeedingMoreWorkers(nullptr) { }
StockPile::StockPile(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : 
	m_openBlocks(data["openBlocks"].get<uint32_t>()), 
	m_area(area), m_faction(deserializationMemo.faction(data["faction"].get<std::wstring>())), 
	m_enabled(data["enabled"].get<bool>()), m_reenableScheduledEvent(m_area.m_simulation.m_eventSchedule), 
	m_projectNeedingMoreWorkers(data.contains("projectNeedingMoreWorkers") ? &static_cast<StockPileProject&>(*deserializationMemo.m_projects.at(data["projectNeedingMoreWorkers"].get<uintptr_t>())) : nullptr) 
{ 
	deserializationMemo.m_stockpiles[data["address"].get<uintptr_t>()] = this;
	for(const Json& blockData : data["blocks"])
		addBlock(deserializationMemo.blockReference(blockData));
	for(const Json& queryData : data["queries"])
		m_queries.emplace_back(queryData, deserializationMemo);
}
Json StockPile::toJson() const
{
	Json data{
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"openBlocks", m_openBlocks},
		{"faction", m_faction.name},
		{"enabled", m_enabled},
		{"queries", Json::array()}
	};
	for(const ItemQuery& query : m_queries)
		data["queries"].push_back(query.toJson());
	if(m_projectNeedingMoreWorkers)
		data["projectNeedingMoreWorkers"] =  reinterpret_cast<uintptr_t>(m_projectNeedingMoreWorkers);
	data["blocks"] = Json::array();
	for(const Block* block : m_blocks)
		data["blocks"].push_back(block->positionToJson());
	return data;
}
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
	for(Item* item : block.m_hasItems.getAll())
		if(accepts(*item))
			m_area.m_hasStocks.at(m_faction).record(*item);
}
void StockPile::removeBlock(Block& block)
{
	assert(m_blocks.contains(&block));
	assert(block.m_isPartOfStockPiles.contains(m_faction));
	assert(block.m_isPartOfStockPiles.getForFaction(m_faction) == this);
	// Collect projects delivering items to block.
	// This is very slow, particularly when destroying a large stockpile when many stockpile projects exist. Probably doesn't get called often enough to matter.
	std::vector<Project*> projectsToCancel;
	for(auto& pair : block.m_area->m_hasStockPiles.at(m_faction).m_projectsByItem)
		for(Project& project : pair.second)
			if(project.getLocation() == block)
				projectsToCancel.push_back(&project);
	block.m_isPartOfStockPiles.recordNoLongerMember(*this);
	if(block.m_isPartOfStockPiles.isAvalible(m_faction))
		decrementOpenBlocks();
	m_blocks.erase(&block);
	if(m_blocks.empty())
		m_area.m_hasStockPiles.at(m_faction).destroyStockPile(*this);
	// Cancel collected projects.
	for(Project* project : projectsToCancel)
		project->cancel();
	for(Item* item : block.m_hasItems.getAll())
		if(accepts(*item))
			m_area.m_hasStocks.at(m_faction).unrecord(*item);
}
void StockPile::updateQueries(std::vector<ItemQuery>& queries)
{
	m_queries = queries;
	//TODO: cancel projects targeting items in this stockpile which now match? Do we need to notify ItemCanBeStockPiled?
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
void StockPile::destroy()
{
	std::vector<Block*> blocks(m_blocks.begin(), m_blocks.end());
	for(Block* block : blocks)
		removeBlock(*block);
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
AreaHasStockPilesForFaction::AreaHasStockPilesForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const Faction& f) :
	m_area(a), m_faction(f)
{
	if(data.contains("stockpiles"))
		for(const Json& stockPileData : data["stockpiles"])
			m_stockPiles.emplace_back(stockPileData, deserializationMemo, m_area);
	if(data.contains("availableStockPilesByItemType"))
		for(const Json& pair : data["availableStockPilesByItemType"])
		{
			const ItemType& itemType = ItemType::byName(pair[0].get<std::string>());
			for(const Json& stockPileAddress : pair[1])
			{
				StockPile& stockPile = *deserializationMemo.m_stockpiles.at(stockPileAddress.get<uintptr_t>());
				m_availableStockPilesByItemType[&itemType].insert(&stockPile);
			}
		}
	if(data.contains("itemsWithoutDestinationsByItemType"))
		for(const Json& pair : data["itemsWithoutDestinationsByItemType"])
		{
			const ItemType& itemType = ItemType::byName(pair[0].get<std::string>());
			for(const Json& itemId : pair[1])
			{
				Item& item = deserializationMemo.m_simulation.getItemById(itemId.get<ItemId>());
				m_itemsWithoutDestinationsByItemType[&itemType].insert(&item);
			}
		}
	if(data.contains("itemsWithDestinationsWithoutProjects"))
		for(const Json& itemId : data["itemsWithDestinationsWithoutProjects"])
		{
			Item& item = deserializationMemo.m_simulation.getItemById(itemId.get<ItemId>());
			m_itemsWithDestinationsWithoutProjects.insert(&item);
		}
	if(data.contains("itemsWithDestinationsByStockPile"))
		for(const Json& pair : data["itemsWithDestinationsByStockPile"])
		{
			StockPile& stockPile = *deserializationMemo.m_stockpiles.at(pair[0].get<uintptr_t>());
			for(const Json& itemId : pair[1])
			{
				Item& item = deserializationMemo.m_simulation.getItemById(itemId.get<ItemId>());
				m_itemsWithDestinationsByStockPile[&stockPile].insert(&item);
			}
		}
	if(data.contains("projectsByItem"))
		for(const Json& pair : data["projectsByItem"])
		{
			Item& item = deserializationMemo.m_simulation.getItemById(pair[0].get<ItemId>());
			for(const Json& project : pair[1])
				m_projectsByItem[&item].emplace_back(project, deserializationMemo);
		}
}
void AreaHasStockPilesForFaction::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("projectsByItem"))
		for(const Json& pair : data["projectsByItem"])
		{
			for(const Json& projectData : pair[1])
			{
				assert(deserializationMemo.m_projects.contains(projectData["address"].get<uintptr_t>()));
				Project& project = *deserializationMemo.m_projects.at(projectData["address"].get<uintptr_t>());
				project.loadWorkers(projectData, deserializationMemo);
			}
		}
}
Json AreaHasStockPilesForFaction::toJson() const
{
	Json data;
	if(!m_stockPiles.empty())
	{
		data["stockpiles"] = Json::array();
		for(const StockPile& stockPile : m_stockPiles)
			data["stockpiles"].push_back(stockPile.toJson());
	}
	if(!m_availableStockPilesByItemType.empty())
	{
		data["availableStockPilesByItemType"] = Json::array();
		for(auto& pair : m_availableStockPilesByItemType)
		{
			Json jsonPair{pair.first->name, Json::array()};
			for(StockPile* stockPile : pair.second)
				jsonPair[1].push_back(reinterpret_cast<uintptr_t>(stockPile));
			data["availableStockPilesByItemType"].push_back(jsonPair);
		}
	}
	if(!m_itemsWithoutDestinationsByItemType.empty())
	{
		data["itemsWithoutDestinationsByItemType"] = Json::array();
		for(auto& pair : m_itemsWithoutDestinationsByItemType)
		{
			Json jsonPair{pair.first->name, Json::array()};
			for(Item* item : pair.second)
				jsonPair[1].push_back(item->m_id);
			data["itemsWithoutDestinationsByItemType"].push_back(jsonPair);
		}
	}
	if(!m_itemsWithDestinationsWithoutProjects.empty())
	{
		data["itemsWithDestinationsWithoutProjects"] = Json::array();
		for(Item* item : m_itemsWithDestinationsWithoutProjects)
			data["itemsWithDestinationsWithoutProjects"].push_back(item->m_id);
	}
	if(!m_itemsWithDestinationsByStockPile.empty())
	{
		data["itemsWithDestinationsByStockPile"] = Json::array();
		for(auto& pair : m_itemsWithDestinationsByStockPile)
		{
			Json jsonPair{reinterpret_cast<uintptr_t>(pair.first), Json::array()};
			for(Item* item : pair.second)
				jsonPair[1].push_back(item->m_id);
			data["itemsWithDestinationsByStockPile"].push_back(jsonPair);
		}
	}
	if(!m_projectsByItem.empty())
	{
		data["projectsByItem"] = Json::array();
		for(auto& pair : m_projectsByItem)
		{
			Json jsonPair{pair.first->m_id, Json::array()};
			for(const StockPileProject& stockPileProject : pair.second)
				jsonPair[1].push_back(stockPileProject.toJson());
			data["projectsByItem"].push_back(jsonPair);
		}
	}

	return data;
}
void AreaHasStockPilesForFaction::destroyStockPile(StockPile& stockPile)
{
	assert(stockPile.m_blocks.empty());
	// Erase pointers from avalible stockpiles by item type.
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].erase(&stockPile);
	// Potentially transfer items from withDestination to withoutDestination, also cancel any associated projects and remove from items with destinations without projects.
	// Sort items which no longer have a potential destination from ones which have a new one.
	if(m_itemsWithDestinationsByStockPile.contains(&stockPile))
		for(Item* item : m_itemsWithDestinationsByStockPile.at(&stockPile))
		{
			StockPile* newStockPile = getStockPileFor(*item);
			assert(newStockPile != &stockPile);
			if(newStockPile == nullptr)
			{
				// Remove all those which no longer have a destination from items with destination without projects.
				// Add them to items without destinaitons by item type.
				if(m_itemsWithDestinationsWithoutProjects.contains(item))
				{
					m_itemsWithDestinationsWithoutProjects.erase(item);
					m_itemsWithoutDestinationsByItemType[&item->m_itemType].insert(item);
				}
			}
			else
				// Record the new stockpile in items with destinations by stockpile.
				m_itemsWithDestinationsByStockPile[newStockPile].insert(item);
		}
	// Remove the stockpile entry from items with destinations by stockpile.
	m_itemsWithDestinationsByStockPile.erase(&stockPile);
	// Destruct.
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
			// We don't need to cancel the stockpile projects here because they will be canceled by dishonorCallback anyway.
		}
	}
}
void AreaHasStockPilesForFaction::removeBlock(Block& block)
{
	StockPile* stockpile = block.m_isPartOfStockPiles.getForFaction(m_faction);
	if(stockpile != nullptr)
		stockpile->removeBlock(block);
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
		StockPile* newStockPile = getStockPileFor(*item);
		if(newStockPile == nullptr)
			m_itemsWithoutDestinationsByItemType[&item->m_itemType].insert(item);
		else
			m_itemsWithDestinationsByStockPile[newStockPile].insert(item);
	}
	m_itemsWithDestinationsByStockPile.erase(&stockPile);
}
void AreaHasStockPilesForFaction::makeProject(Item& item, Block& destination, StockPileObjective& objective)
{
	assert(destination.m_isPartOfStockPiles.contains(*objective.m_actor.getFaction()));
	StockPileProject& project = m_projectsByItem[&item].emplace_back(objective.m_actor.getFaction(), destination, item);
	std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<StockPileHasShapeDishonorCallback>(project);
	project.setLocationDishonorCallback(std::move(dishonorCallback));
	project.addWorkerCandidate(objective.m_actor, objective);
	objective.m_project = &project;
	objective.m_actor.m_project = &project;
}
void AreaHasStockPilesForFaction::cancelProject(StockPileProject& project)
{
	Item& item = project.m_item;
	destroyProject(project);
	addItem(item);
}
void AreaHasStockPilesForFaction::destroyProject(StockPileProject& project)
{
	if(m_projectsByItem.at(&project.m_item).size() == 1)
		m_projectsByItem.erase(&project.m_item);
	else
		m_projectsByItem.at(&project.m_item).remove(project);
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
void AreaHasStockPiles::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0].get<std::wstring>());
		m_data.try_emplace(&faction, pair[1], deserializationMemo, m_area, faction);
	}
}
void AreaHasStockPiles::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.at(&faction).loadWorkers(pair[1], deserializationMemo);
	}
}
Json AreaHasStockPiles::toJson() const 
{
	Json data;
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first->name, pair.second.toJson()};
		data.push_back(jsonPair);
	}
	return data;
}
