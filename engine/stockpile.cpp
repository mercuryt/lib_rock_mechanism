#include "stockpile.h"
#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "items/itemQuery.h"
#include "reservable.h"
#include "simulation.h"
#include "stocks.h"
#include "terrainFacade.h"
#include "types.h"
#include "itemType.h"
#include <cwchar>
#include <functional>
#include <memory>
#include <string>
/*
//Input
void StockPileCreateInputAction::execute()
{
	auto& hasStockPiles = (*m_cuboid.begin()).m_area->m_hasStockPiles.at(m_faction);
	StockPile& stockpile = hasStockPiles.addStockPile(m_queries);
	for(BlockIndex block : m_cuboid)
		stockpile.addBlock(block);
}
void StockPileRemoveInputAction::execute()
{
	auto& hasStockPiles = (*m_cuboid.begin()).m_area->m_hasStockPiles.at(m_faction);
	for(BlockIndex block : m_cuboid)
		hasStockPiles.removeBlock(block);
}
void StockPileExpandInputAction::execute()
{
	for(BlockIndex block : m_cuboid)
		m_stockpile.addBlock(block);
}
void StockPileUpdateInputAction::execute()
{
	m_stockpile.updateQueries(m_queries);
}
*/
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
StockPilePathRequest::StockPilePathRequest(Area& area, StockPileObjective& spo) : m_objective(spo)
{
	assert(m_objective.m_project == nullptr);
	assert(m_objective.m_destination == BLOCK_INDEX_MAX);
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	assert(!actors.project_exists(m_objective.m_actor));
	Faction& faction = *actors.getFaction(spo.m_actor);
	if(m_objective.m_item == BLOCK_INDEX_MAX)
	{
		auto& hasStockPiles = area.m_hasStockPiles.at(faction);
		std::function<bool(BlockIndex)> blocksContainsItemCondition = [this, &hasStockPiles, &items](BlockIndex block)
		{
			ItemIndex item = hasStockPiles.getHaulableItemForAt(m_objective.m_actor, block);
			if(item == ITEM_INDEX_MAX)
				return false;
			std::tuple<const ItemType*, const MaterialType*> tuple = {&items.getItemType(item), &items.getMaterialType(item)};
			return std::ranges::find(m_objective.m_closedList, tuple) == m_objective.m_closedList.end();
		};
		bool unreserved = false;
		DistanceInBlocks maxRange = Config::maxRangeToSearchForStockPileItems;
		createGoAdjacentToCondition(area, m_objective.m_actor, blocksContainsItemCondition, m_objective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
	}
	else
	{
		std::function<bool(BlockIndex)> condition = [this, &area](BlockIndex block) { return m_objective.destinationCondition(area, block, m_objective.m_item); };
		bool unreserved = false;
		DistanceInBlocks maxRange = Config::maxRangeToSearchForStockPiles;
		createGoAdjacentToCondition(area, m_objective.m_actor, condition, m_objective.m_detour, unreserved, maxRange, BLOCK_INDEX_MAX);
	}
}
void StockPilePathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	Faction& faction = *actors.getFaction(m_objective.m_actor);
	auto& hasStockPiles = area.m_hasStockPiles.at(faction);
	if(m_objective.m_item == ITEM_INDEX_MAX)
	{
		if(result.path.empty() && !result.useCurrentPosition)
			// No haulable item found.
			actors.objective_canNotCompleteObjective(m_objective.m_actor, m_objective);
		else
		{
			ItemIndex item = hasStockPiles.getHaulableItemForAt(m_objective.m_actor, result.blockThatPassedPredicate);
			if(item != ITEM_INDEX_MAX)
				m_objective.m_item = item;
			m_objective.execute(area);
		}
	}
	else
	{
		if(result.path.empty() && !result.useCurrentPosition)
		{
			// No stockpile found.
			Items& items = area.getItems();
			m_objective.m_closedList.emplace_back(&items.getItemType(m_objective.m_item), &items.getMaterialType(m_objective.m_item));
			m_objective.execute(area);
		}
		else
		{
			if(!m_objective.destinationCondition(area, result.blockThatPassedPredicate, m_objective.m_item))
				// Found destination is no longer valid.
				m_objective.execute(area);
			else
			{
				// Destination found, join or create a project.
				Faction& faction = *actors.getFaction(m_objective.m_actor);
				Blocks& blocks = area.getBlocks();
				assert(blocks.stockpile_getForFaction(m_objective.m_destination, faction));
				StockPile& stockpile = *blocks.stockpile_getForFaction(m_objective.m_destination, faction);
				if(stockpile.hasProjectNeedingMoreWorkers())
					stockpile.addToProjectNeedingMoreWorkers(m_objective.m_actor, m_objective);
				else
				{
					auto& hasStockPiles = area.m_hasStockPiles.at(faction);
					if(hasStockPiles.m_projectsByItem.contains(m_objective.m_item))
					{
						// Projects found, select one to join.
						for(StockPileProject& stockPileProject : hasStockPiles.m_projectsByItem.at(m_objective.m_item))
							if(stockPileProject.canAddWorker(m_objective.m_actor))
							{
								m_objective.m_project = &stockPileProject;
								stockPileProject.addWorkerCandidate(m_objective.m_actor, m_objective);
							}
					}
					else
						// No projects found, make one.
						hasStockPiles.makeProject(m_objective.m_item, m_objective.m_destination, m_objective);
				}
			}
		}
	}
}
bool StockPileObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	return area.m_hasStockPiles.at(*area.getActors().getFaction(actor)).isAnyHaulingAvailableFor(actor);
}
std::unique_ptr<Objective> StockPileObjectiveType::makeFor(Area&, ActorIndex actor) const
{
	return std::make_unique<StockPileObjective>(actor);
}
StockPileObjective::StockPileObjective(ActorIndex a) :
	Objective(a, Config::stockPilePriority) { }
/*
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
*/
void StockPileObjective::execute(Area& area) 
{ 
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
	{
		Actors& actors = area.getActors();
		assert(!actors.project_exists(m_actor));
		actors.move_pathRequestRecord(m_actor, std::make_unique<StockPilePathRequest>(area, *this));
	}
	else
		m_project->commandWorker(m_actor);
}
void StockPileObjective::cancel(Area& area)
{
	Actors& actors = area.getActors();
	if(m_project != nullptr)
	{
		m_project->removeWorker(m_actor);
		m_project = nullptr;
		actors.project_unset(m_actor);
	}
	else
		assert(!actors.project_exists(m_actor));
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.canReserve_clearAll(m_actor);
}
bool StockPileObjective::destinationCondition(Area& area, BlockIndex block, const ItemIndex item)
{
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	assert(m_destination == BLOCK_INDEX_MAX);
	if(!blocks.shape_staticCanEnterCurrentlyWithAnyFacing(block, items.getShape(item), items.getBlocks(item)))
		return false;
	if(!blocks.item_empty(block))
		// Don't put multiple items in the same block unless they are generic and share item type and material type.
		if(!items.isGeneric(item) || !blocks.item_getCount(block, items.getItemType(item), items.getMaterialType(item)))
			return false;
	Faction& faction = *actors.getFaction(m_actor);
	const StockPile* stockpile = blocks.stockpile_getForFaction(block, faction);
	if(stockpile == nullptr || !stockpile->isEnabled() || !blocks.stockpile_isAvalible(block, faction))
		return false;
	if(blocks.isReserved(block, faction))
		return false;
	if(!stockpile->accepts(item))
		return false;
	m_destination = block;
	return true;
};
StockPileProject::StockPileProject(Faction* faction, Area& area, BlockIndex block, ItemIndex item, Quantity quantity, Quantity maxWorkers) : 
	Project(faction, area, block, maxWorkers), 
	m_item(item), m_quantity(quantity),
       	m_itemType(area.getItems().getItemType(item)), 
	m_materialType(area.getItems().getMaterialType(item)), 
	m_stockpile(*area.getBlocks().stockpile_getForFaction(block, *faction)) { }
/*
StockPileProject::StockPileProject(const Json& data, DeserializationMemo& deserializationMemo) : 
	Project(data, deserializationMemo),
	m_item(deserializationMemo.m_simulation.m_hasItems->getById(data["item"].get<ItemId>())), 
	m_quantity(data["quantity"].get<Quantity>()),
	m_itemType(ItemType::byName(data["itemType"].get<std::string>())), 
	m_materialType(MaterialType::byName(data["materialType"].get<std::string>())), 
	m_stockpile(*deserializationMemo.m_stockpiles.at(data["stockpile"].get<uintptr_t>())) { }
Json StockPileProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item.m_id;
	data["stockpile"] = reinterpret_cast<uintptr_t>(&m_stockpile);
	data["quantity"] = m_quantity;
	data["itemType"] = m_itemType;
	data["materialType"] = m_materialType;
	return data;
}
*/
void StockPileProject::onComplete()
{
	Items& items = m_area.getItems();
	for(ItemIndex item : m_deliveredItems)
		items.setLocation(item, m_location);
	for(auto& pair : m_alreadyAtSite)
		pair.first.setLocationAndFacing(m_area, m_location, 0);
	Blocks& blocks = m_area.getBlocks();
	ItemIndex delivered = blocks.item_getGeneric(m_location, m_itemType, m_materialType);
	auto workers = std::move(m_workers);
	auto& hasStockPiles = m_area.m_hasStockPiles.at(m_stockpile.m_faction);
	m_area.m_hasStocks.at(m_faction).record(delivered);
	if(delivered == m_item)
		// Either non-generic or whole stack.
		hasStockPiles.removeItem(m_item);
	else
		// Partial stack of generic.
		hasStockPiles.destroyProject(*this);
	Actors& actors = m_area.getActors();
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor, projectWorker.objective);
}
void StockPileProject::onReserve()
{
	// Project has reserved it's item and destination, as well as haul tool and / or beast of burden.
	// Clear from StockPile::m_projectNeedingMoreWorkers.
	if(m_stockpile.m_projectNeedingMoreWorkers == this)
		m_stockpile.m_projectNeedingMoreWorkers = nullptr;
}
void StockPileProject::onCancel()
{
	std::vector<ActorIndex> workersAndCandidates = getWorkersAndCandidates();
	m_area.m_hasStockPiles.at(m_faction).destroyProject(*this);
	Actors& actors = m_area.getActors();
	for(ActorIndex actor : workersAndCandidates)
	{
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(actor);
		actors.project_unset(actor);
		objective.m_project = nullptr;
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void StockPileProject::onDelay() 
{ 
	m_area.getItems().stockpile_maybeUnsetAndScheduleReset(m_item, m_faction, Config::stepsToDisableStockPile); 
	cancel(); 
}
void StockPileProject::onHasShapeReservationDishonored(ActorOrItemIndex, Quantity, Quantity newCount)
{
	if(newCount == 0)
		cancel();
}
bool StockPileProject::canAddWorker(const ActorIndex actor) const
{
	if(!Project::canAddWorker(actor))
		return false;
	return m_haulSubprojects.empty();
}
std::vector<std::pair<ItemQuery, Quantity>> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, Quantity>> StockPileProject::getUnconsumed() const { return {{m_item, m_quantity}}; }
std::vector<std::tuple<const ItemType*, const MaterialType*, Quantity>> StockPileProject::getByproducts() const {return {}; }
std::vector<std::pair<ActorQuery, Quantity>> StockPileProject::getActors() const { return {}; }
StockPile::StockPile(std::vector<ItemQuery>& q, Area& a, Faction& f) :
	m_queries(q), m_area(a), m_faction(f), m_reenableScheduledEvent(m_area.m_eventSchedule) { }
/*
StockPile::StockPile(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : 
	m_openBlocks(data["openBlocks"].get<Quantity>()), 
	m_area(area), m_faction(deserializationMemo.faction(data["faction"].get<std::wstring>())), 
	m_enabled(data["enabled"].get<bool>()), m_reenableScheduledEvent(m_area.m_simulation.m_eventSchedule), 
	m_projectNeedingMoreWorkers(data.contains("projectNeedingMoreWorkers") ? &static_cast<StockPileProject&>(*deserializationMemo.m_projects.at(data["projectNeedingMoreWorkers"].get<uintptr_t>())) : nullptr) 
{ 
	deserializationMemo.m_stockpiles[data["address"].get<uintptr_t>()] = this;
	for(const Json& block : data["blocks"])
		addBlock(block.get<BlockIndex>());
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
	for(BlockIndex block : m_blocks)
		data["blocks"].push_back(block);
	return data;
}
*/
bool StockPile::accepts(const ItemIndex item) const 
{
	for(const ItemQuery& itemQuery : m_queries)
		if(itemQuery.query(m_area, item))
			return true;
	return false;
}
void StockPile::addBlock(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(!m_blocks.contains(block));
	assert(!blocks.stockpile_contains(block, m_faction));
	blocks.stockpile_recordMembership(block, *this);
	if(blocks.stockpile_contains(block, m_faction))
		incrementOpenBlocks();
	m_blocks.insert(block);
	Items& items = m_area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
		if(accepts(item))
		{
			m_area.m_hasStocks.at(m_faction).record(item);
			if(items.stockpile_canBeStockPiled(item, m_faction))
				m_area.m_hasStockPiles.at(m_faction).removeItem(item);
		}
}
void StockPile::removeBlock(BlockIndex block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(m_blocks.contains(block));
	assert(blocks.stockpile_contains(block, m_faction));
	assert(blocks.stockpile_getForFaction(block, m_faction) == this);
	// Collect projects delivering items to block.
	// This is very slow, particularly when destroying a large stockpile when many stockpile projects exist. Probably doesn't get called often enough to matter.
	std::vector<Project*> projectsToCancel;
	for(auto& pair : m_area.m_hasStockPiles.at(m_faction).m_projectsByItem)
		for(Project& project : pair.second)
			if(project.getLocation() == block)
				projectsToCancel.push_back(&project);
	if(blocks.stockpile_isAvalible(block, m_faction))
		decrementOpenBlocks();
	blocks.stockpile_recordNoLongerMember(block, *this);
	m_blocks.erase(block);
	if(m_blocks.empty())
		m_area.m_hasStockPiles.at(m_faction).destroyStockPile(*this);
	// Cancel collected projects.
	for(Project* project : projectsToCancel)
		project->cancel();
	for(ItemIndex item : blocks.item_getAll(block))
		if(accepts(item))
			m_area.m_hasStocks.at(m_faction).unrecord(item);
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
	return m_area.m_simulation;
}
void StockPile::addToProjectNeedingMoreWorkers(ActorIndex actor, StockPileObjective& objective)
{
	assert(m_projectNeedingMoreWorkers != nullptr);
	m_projectNeedingMoreWorkers->addWorkerCandidate(actor, objective);
}
void StockPile::destroy()
{
	std::vector<BlockIndex> blocks(m_blocks.begin(), m_blocks.end());
	for(BlockIndex block : blocks)
		removeBlock(block);
}
bool StockPile::contains(ItemQuery& query) const
{
	return std::find(m_queries.begin(), m_queries.end(), query) != m_queries.end();
}
bool StockPile::hasProjectNeedingMoreWorkers() const 
{ 
	assert(this);
	return m_projectNeedingMoreWorkers != nullptr;
}
void StockPile::addQuery(ItemQuery& query)
{
	assert(!contains(query));
	m_queries.push_back(query);
}
void StockPile::removeQuery(ItemQuery& query)
{
	assert(contains(query));
	std::erase_if(m_queries, [&query](const ItemQuery& q){ return &q == &query; });
}
StockPileHasShapeDishonorCallback::StockPileHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_stockPileProject(static_cast<StockPileProject&>(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
StockPile& AreaHasStockPilesForFaction::addStockPile(std::vector<ItemQuery>&& queries) { return addStockPile(queries); }
StockPile& AreaHasStockPilesForFaction::addStockPile(std::vector<ItemQuery>& queries)
{
	StockPile& stockPile = m_stockPiles.emplace_back(queries, m_area, m_faction);
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].insert(&stockPile);
	return stockPile;
}
/*
AreaHasStockPilesForFaction::AreaHasStockPilesForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, Faction& f) :
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
				ItemIndex item = deserializationMemo.m_simulation.m_hasItems->getById(itemId.get<ItemId>());
				m_itemsWithoutDestinationsByItemType[&itemType].insert(item);
			}
		}
	if(data.contains("itemsWithDestinationsWithoutProjects"))
		for(const Json& itemId : data["itemsWithDestinationsWithoutProjects"])
		{
			ItemIndex item = deserializationMemo.m_simulation.m_hasItems->getById(itemId.get<ItemId>());
			m_itemsWithDestinationsWithoutProjects.insert(item);
		}
	if(data.contains("itemsWithDestinationsByStockPile"))
		for(const Json& pair : data["itemsWithDestinationsByStockPile"])
		{
			StockPile& stockPile = *deserializationMemo.m_stockpiles.at(pair[0].get<uintptr_t>());
			for(const Json& itemId : pair[1])
			{
				ItemIndex item = deserializationMemo.m_simulation.m_hasItems->getById(itemId.get<ItemId>());
				m_itemsWithDestinationsByStockPile[&stockPile].insert(item);
			}
		}
	if(data.contains("projectsByItem"))
		for(const Json& pair : data["projectsByItem"])
		{
			ItemIndex item = deserializationMemo.m_simulation.m_hasItems->getById(pair[0].get<ItemId>());
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
			for(ItemIndex item : pair.second)
				jsonPair[1].push_back(item->m_id);
			data["itemsWithoutDestinationsByItemType"].push_back(jsonPair);
		}
	}
	if(!m_itemsWithDestinationsWithoutProjects.empty())
	{
		data["itemsWithDestinationsWithoutProjects"] = Json::array();
		for(ItemIndex item : m_itemsWithDestinationsWithoutProjects)
			data["itemsWithDestinationsWithoutProjects"].push_back(item->m_id);
	}
	if(!m_itemsWithDestinationsByStockPile.empty())
	{
		data["itemsWithDestinationsByStockPile"] = Json::array();
		for(auto& pair : m_itemsWithDestinationsByStockPile)
		{
			Json jsonPair{reinterpret_cast<uintptr_t>(pair.first), Json::array()};
			for(ItemIndex item : pair.second)
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
*/
void AreaHasStockPilesForFaction::destroyStockPile(StockPile& stockPile)
{
	assert(stockPile.m_blocks.empty());
	// Erase pointers from avalible stockpiles by item type.
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].erase(&stockPile);
	Items& items = m_area.getItems();
	// Potentially transfer items from withDestination to withoutDestination, also cancel any associated projects and remove from items with destinations without projects.
	// Sort items which no longer have a potential destination from ones which have a new one.
	if(m_itemsWithDestinationsByStockPile.contains(&stockPile))
		for(ItemIndex item : m_itemsWithDestinationsByStockPile.at(&stockPile))
		{
			StockPile* newStockPile = getStockPileFor(item);
			assert(newStockPile != &stockPile);
			if(newStockPile == nullptr)
			{
				// Remove all those which no longer have a destination from items with destination without projects.
				// Add them to items without destinaitons by item type.
				if(m_itemsWithDestinationsWithoutProjects.contains(item))
				{
					m_itemsWithDestinationsWithoutProjects.erase(item);
					m_itemsWithoutDestinationsByItemType[&items.getItemType(item)].insert(item);
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
bool AreaHasStockPilesForFaction::isValidStockPileDestinationFor(BlockIndex block, const ItemIndex item) const
{
	Blocks& blocks = m_area.getBlocks();
	if(blocks.isReserved(block, m_faction))
		return false;
	if(!blocks.stockpile_isAvalible(block, m_faction))
		return false;
	return blocks.stockpile_getForFaction(block, m_faction)->accepts(item);
}
void AreaHasStockPilesForFaction::addItem(ItemIndex item)
{
	Items& items = m_area.getItems();
	items.stockpile_set(item, m_faction);
	// This isn't neccessarily the stockpile that the item is going to, it is only the one that proved that there exists a stockpile somewhere which accepts this item.
	StockPile* stockPile = getStockPileFor(item);
	if(stockPile == nullptr)
		m_itemsWithoutDestinationsByItemType[&items.getItemType(item)].insert(item);
	else
	{
		m_itemsWithDestinationsByStockPile[stockPile].insert(item);
		m_itemsWithDestinationsWithoutProjects.insert(item);
	}
}
void AreaHasStockPilesForFaction::maybeAddItem(ItemIndex item)
{
	const ItemType& itemType = m_area.getItems().getItemType(item);
	if(!m_projectsByItem.contains(item) &&
		(
			!m_itemsWithoutDestinationsByItemType.contains(&itemType) || 
			!m_itemsWithoutDestinationsByItemType.at(&itemType).contains(item)
		)
	) 
		addItem(item);
}
void AreaHasStockPilesForFaction::removeItem(ItemIndex item)
{
	Items& items = m_area.getItems();
	const ItemType& itemType = m_area.getItems().getItemType(item);
	items.stockpile_maybeUnset(item, m_faction);
	if(m_itemsWithoutDestinationsByItemType.contains(&itemType) && m_itemsWithoutDestinationsByItemType.at(&itemType).contains(item))
		m_itemsWithoutDestinationsByItemType.at(&itemType).erase(item);
	else
	{
		m_itemsWithDestinationsWithoutProjects.erase(item);
		if(m_projectsByItem.contains(item))
			for(StockPileProject& stockPileProject : m_projectsByItem.at(item))
				removeFromItemsWithDestinationByStockPile(stockPileProject.m_stockpile, item);
			// We don't need to cancel the stockpile projects here because they will be canceled by dishonorCallback anyway.
	}
}
void AreaHasStockPilesForFaction::removeFromItemsWithDestinationByStockPile(const StockPile& stockPile, const ItemIndex item)
{
	StockPile* nonConst = const_cast<StockPile*>(&stockPile);
	if(m_itemsWithDestinationsByStockPile.at(nonConst).size() == 1)
		m_itemsWithDestinationsByStockPile.erase(nonConst);
	else
		m_itemsWithDestinationsByStockPile.at(nonConst).erase(item);
}
void AreaHasStockPilesForFaction::removeBlock(BlockIndex block)
{
	StockPile* stockpile = m_area.getBlocks().stockpile_getForFaction(block, m_faction);
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
			for(ItemIndex item : m_itemsWithoutDestinationsByItemType.at(itemQuery.m_itemType))
				if(stockPile.accepts(item))
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
	if(m_itemsWithDestinationsByStockPile.contains(&stockPile))
	{
		for(ItemIndex item : m_itemsWithDestinationsByStockPile.at(&stockPile))
		{
			StockPile* newStockPile = getStockPileFor(item);
			if(newStockPile == nullptr)
				m_itemsWithoutDestinationsByItemType[&m_area.getItems().getItemType(item)].insert(item);
			else
				m_itemsWithDestinationsByStockPile[newStockPile].insert(item);
		}
		m_itemsWithDestinationsByStockPile.erase(&stockPile);
	}
}
void AreaHasStockPilesForFaction::makeProject(ItemIndex item, BlockIndex destination, StockPileObjective& objective)
{
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	Faction& faction = *actors.getFaction(objective.m_actor);
	assert(blocks.stockpile_contains(destination, faction));
	// Quantity per project is the ammount that can be hauled by hand, if any, or one.
	// TODO: Hauling a load of generics with tools.
	Quantity quantity = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(objective.m_actor, item, Config::minimumHaulSpeedInital);
	Quantity maxWorkers = 1;
	if(!quantity)
	{
		quantity = 1;
		maxWorkers = 2;
	}
	StockPileProject& project = m_projectsByItem[item].emplace_back(faction, m_area, destination, item, quantity, maxWorkers);
	std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<StockPileHasShapeDishonorCallback>(project);
	project.setLocationDishonorCallback(std::move(dishonorCallback));
	project.addWorkerCandidate(objective.m_actor, objective);
	objective.m_project = &project;
	actors.project_set(objective.m_actor, project);
	m_itemsWithDestinationsWithoutProjects.erase(item);
}
void AreaHasStockPilesForFaction::cancelProject(StockPileProject& project)
{
	ItemIndex item = project.m_item;
	destroyProject(project);
	addItem(item);
}
void AreaHasStockPilesForFaction::destroyProject(StockPileProject& project)
{
	if(m_projectsByItem.at(project.m_item).size() == 1)
		m_projectsByItem.erase(project.m_item);
	else
		m_projectsByItem.at(project.m_item).remove(project);
}

void AreaHasStockPilesForFaction::addQuery(StockPile& stockPile, ItemQuery query)
{
	stockPile.addQuery(query);
	assert(query.m_itemType);
	m_availableStockPilesByItemType[query.m_itemType].insert(&stockPile);
}
void AreaHasStockPilesForFaction::removeQuery(StockPile& stockPile, ItemQuery query)
{
	stockPile.removeQuery(query);
	assert(query.m_itemType);
	assert(m_availableStockPilesByItemType.contains(query.m_itemType));
	if(m_availableStockPilesByItemType.at(query.m_itemType).size() == 1)
		m_availableStockPilesByItemType.erase(query.m_itemType);
	else
		m_availableStockPilesByItemType[query.m_itemType].insert(&stockPile);
}
bool AreaHasStockPilesForFaction::isAnyHaulingAvailableFor(const ActorIndex actor) const
{
	assert(&m_faction == m_area.getActors().getFaction(actor));
	return !m_itemsWithDestinationsByStockPile.empty();
}
ItemIndex AreaHasStockPilesForFaction::getHaulableItemForAt(const ActorIndex actor, BlockIndex block)
{
	assert(m_area.getActors().getFaction(actor) != nullptr);
	Blocks& blocks = m_area.getBlocks();
	Faction& faction = *m_area.getActors().getFaction(actor);
	Items& items = m_area.getItems();
	if(blocks.isReserved(block, faction))
		return ITEM_INDEX_MAX;
	for(ItemIndex item : blocks.item_getAll(block))
	{
		if(items.reservable_isFullyReserved(item, faction))
			continue;
		if(items.stockpile_canBeStockPiled(item, faction))
			return item;
	}
	return ITEM_INDEX_MAX;
}
StockPile* AreaHasStockPilesForFaction::getStockPileFor(const ItemIndex item) const
{
	const ItemType& itemType = m_area.getItems().getItemType(item);
	if(!m_availableStockPilesByItemType.contains(&itemType))
		return nullptr;
	for(StockPile* stockPile : m_availableStockPilesByItemType.at(&itemType))
		if(stockPile->accepts(item))
			return stockPile;
	return nullptr;
}
AreaHasStockPilesForFaction& AreaHasStockPiles::at(Faction& faction) 
{ 
	if(!m_data.contains(&faction))
		registerFaction(faction);
	return m_data.at(&faction); 
}
void AreaHasStockPiles::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Faction& faction = deserializationMemo.faction(pair[0].get<std::wstring>());
		m_data.try_emplace(&faction, pair[1], deserializationMemo, m_area, faction);
	}
}
void AreaHasStockPiles::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		Faction& faction = deserializationMemo.faction(pair[0]);
		m_data.at(&faction).loadWorkers(pair[1], deserializationMemo);
	}
}
void AreaHasStockPiles::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair : pair.second.m_projectsByItem)
			for(auto& project : pair.second)
				project.clearReservations();
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
