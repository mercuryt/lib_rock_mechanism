#include "stockpile.h"
#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "items/itemQuery.h"
#include "items/items.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "objectives/stockpile.h"
#include "reference.h"
#include "reservable.h"
#include "simulation.h"
#include "stocks.h"
#include "terrainFacade.h"
#include "types.h"
#include "itemType.h"
#include "util.h"
#include <cwchar>
#include <functional>
#include <memory>
#include <string>
/*
//Input
void StockPileCreateInputAction::execute()
{
	auto& hasStockPiles = (*m_cuboid.begin()).m_area->m_hasStockPiles.getForFaction(m_faction);
	StockPile& stockpile = hasStockPiles.addStockPile(m_queries);
	for(BlockIndex block : m_cuboid)
		stockpile.addBlock(block);
}
void StockPileRemoveInputAction::execute()
{
	auto& hasStockPiles = (*m_cuboid.begin()).m_area->m_hasStockPiles.getForFaction(m_faction);
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
StockPileProject::StockPileProject(const FactionId& faction, Area& area, const BlockIndex& block, const ItemIndex& item, const Quantity& quantity, const Quantity& maxWorkers) :
	Project(faction, area, block, maxWorkers),
	m_item(item, area.getItems().m_referenceData),
	m_quantity(quantity),
	m_itemType(area.getItems().getItemType(item)),
	m_materialType(area.getItems().getMaterialType(item)),
	m_stockpile(*area.getBlocks().stockpile_getForFaction(block, faction)) { }
StockPileProject::StockPileProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Project(data, deserializationMemo, area),
	m_item(data["item"], area.getItems().m_referenceData),
	m_quantity(data["quantity"].get<Quantity>()),
	m_itemType(ItemType::byName(data["itemType"].get<std::wstring>())),
	m_materialType(MaterialType::byName(data["materialType"].get<std::wstring>())),
	m_stockpile(*deserializationMemo.m_stockpiles.at(data["stockpile"].get<uintptr_t>())) { }
Json StockPileProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item;
	data["stockpile"] = reinterpret_cast<uintptr_t>(&m_stockpile);
	data["quantity"] = m_quantity;
	data["itemType"] = m_itemType;
	data["materialType"] = m_materialType;
	return data;
}
void StockPileProject::onComplete()
{
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	assert(m_deliveredItems.size() == 1 || m_alreadyAtSite.size() == 1);
	ItemIndex index = m_item.getIndex(items.m_referenceData);
	auto workers = std::move(m_workers);
	FactionId faction = m_stockpile.m_faction;
	Area& area = m_area;
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	BlockIndex location = m_location;
	hasStockPiles.maybeRemoveFromItemsWithDestinationByStockPile(m_stockpile, index);
	hasStockPiles.destroyProject(*this);
	index = items.setLocationAndFacing(index, location, Facing4::North);
	area.m_hasStocks.getForFaction(faction).maybeRecord(area, index);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
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
	ActorIndices workersAndCandidates = getWorkersAndCandidates();
	Actors& actors = m_area.getActors();
	Area& area = m_area;
	// This is destroyed here.
	m_area.m_hasStockPiles.getForFaction(m_faction).destroyProject(*this);
	for(ActorIndex actor : workersAndCandidates)
	{
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(actor);
		// clear objective.m_project so reset does not try to cancel it.
		objective.m_project = nullptr;
		objective.reset(area, actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void StockPileProject::onDelay()
{
	Items& items = m_area.getItems();
	m_area.getItems().stockpile_maybeUnsetAndScheduleReset(m_item.getIndex(items.m_referenceData), m_faction, Config::stepsToDisableStockPile);
	cancel();
}
void StockPileProject::onPickUpRequired(const ActorOrItemIndex& required)
{
	ItemReference ref = m_area.getItems().getReference(required.getItem());
	updateRequiredGenericReference(ref);
}
void StockPileProject::updateRequiredGenericReference(const ItemReference& newRef)
{
	if(newRef != m_item)
	{
		m_area.m_hasStockPiles.getForFaction(m_faction).updateItemReferenceForProject(*this, newRef);
		m_item = newRef;
	}
}
bool StockPileProject::canAddWorker(const ActorIndex& actor) const
{
	if(!Project::canAddWorker(actor))
		return false;
	return m_haulSubprojects.empty();
}
std::vector<std::pair<ItemQuery, Quantity>> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, Quantity>> StockPileProject::getUnconsumed() const { return {{ItemQuery::create(m_item), m_quantity}}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> StockPileProject::getByproducts() const {return {}; }
std::vector<std::pair<ActorQuery, Quantity>> StockPileProject::getActors() const { return {}; }
StockPile::StockPile(std::vector<ItemQuery>& q, Area& a, const FactionId& f) :
	m_queries(q), m_area(a), m_faction(f), m_reenableScheduledEvent(m_area.m_eventSchedule) { }
StockPile::StockPile(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	m_openBlocks(data["openBlocks"].get<Quantity>()),
	m_area(area), m_faction(data["faction"].get<FactionId>()),
	m_enabled(data["enabled"].get<bool>()), m_reenableScheduledEvent(m_area.m_simulation.m_eventSchedule),
	m_projectNeedingMoreWorkers(data.contains("projectNeedingMoreWorkers") ? &static_cast<StockPileProject&>(*deserializationMemo.m_projects.at(data["projectNeedingMoreWorkers"].get<uintptr_t>())) : nullptr)
{
	deserializationMemo.m_stockpiles[data["address"].get<uintptr_t>()] = this;
	for(const Json& queryData : data["queries"])
		m_queries.emplace_back(queryData, area);
	for(const Json& blockData : data["blocks"]["data"])
		addBlock(blockData.get<BlockIndex>());
}
Json StockPile::toJson() const
{
	Json data{
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"openBlocks", m_openBlocks},
		{"faction", m_faction},
		{"enabled", m_enabled},
		{"queries", m_queries},
		{"blocks", m_blocks}
	};
	if(m_projectNeedingMoreWorkers)
		data["projectNeedingMoreWorkers"] = reinterpret_cast<uintptr_t>(m_projectNeedingMoreWorkers);
	return data;
}
bool StockPile::accepts(const ItemIndex& item) const
{
	for(const ItemQuery& itemQuery : m_queries)
		if(itemQuery.query(m_area, item))
			return true;
	return false;
}
void StockPile::addBlock(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(!m_blocks.contains(block));
	assert(!blocks.stockpile_contains(block, m_faction));
	blocks.stockpile_recordMembership(block, *this);
	if(blocks.stockpile_contains(block, m_faction))
		incrementOpenBlocks();
	m_blocks.add(block);
	Items& items = m_area.getItems();
	for(ItemIndex item : blocks.item_getAll(block))
		if(accepts(item))
		{
			m_area.m_hasStocks.getForFaction(m_faction).record(m_area, item);
			if(items.stockpile_canBeStockPiled(item, m_faction))
				m_area.m_hasStockPiles.getForFaction(m_faction).removeItem(item);
		}
}
void StockPile::removeBlock(const BlockIndex& block)
{
	Blocks& blocks = m_area.getBlocks();
	assert(m_blocks.contains(block));
	assert(blocks.stockpile_contains(block, m_faction));
	assert(blocks.stockpile_getForFaction(block, m_faction) == this);
	// Collect projects delivering items to block.
	// This is very slow, particularly when destroying a large stockpile when many stockpile projects exist. Probably doesn't get called often enough to matter.
	std::vector<Project*> projectsToCancel;
	for(auto& pair : m_area.m_hasStockPiles.getForFaction(m_faction).m_projectsByItem)
		for(StockPileProject* project : pair.second)
			if(project->getLocation() == block)
				projectsToCancel.push_back(project);
	if(blocks.stockpile_isAvalible(block, m_faction))
		decrementOpenBlocks();
	blocks.stockpile_recordNoLongerMember(block, *this);
	m_blocks.remove(block);
	if(m_blocks.empty())
		m_area.m_hasStockPiles.getForFaction(m_faction).destroyStockPile(*this);
	// Cancel collected projects.
	for(Project* project : projectsToCancel)
		project->cancel();
	for(ItemIndex item : blocks.item_getAll(block))
		if(accepts(item))
			m_area.m_hasStocks.getForFaction(m_faction).unrecord(m_area, item);
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
		m_area.m_hasStockPiles.getForFaction(m_faction).setAvailable(*this);
}
void StockPile::decrementOpenBlocks()
{
	--m_openBlocks;
	if(m_openBlocks == 0)
		m_area.m_hasStockPiles.getForFaction(m_faction).setUnavailable(*this);
}
Simulation& StockPile::getSimulation()
{
	assert(!m_blocks.empty());
	return m_area.m_simulation;
}
void StockPile::addToProjectNeedingMoreWorkers(const ActorIndex& actor, StockPileObjective& objective)
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
	return std::ranges::contains(m_queries, query);
}
bool StockPile::hasProjectNeedingMoreWorkers() const
{
	return m_projectNeedingMoreWorkers != nullptr;
}
void StockPile::addQuery(const ItemQuery& query)
{
	assert(!contains(const_cast<ItemQuery&>(query)));
	m_queries.push_back(query);
}
void StockPile::removeQuery(const ItemQuery& query)
{
	assert(contains(const_cast<ItemQuery&>(query)));
	std::erase_if(m_queries, [&query](const ItemQuery& q){ return &q == &query; });
}
StockPileHasShapeDishonorCallback::StockPileHasShapeDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) :
	m_stockPileProject(static_cast<StockPileProject&>(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>()))) { }
StockPile& AreaHasStockPilesForFaction::addStockPile(std::vector<ItemQuery>&& queries) { return addStockPile(queries); }
StockPile& AreaHasStockPilesForFaction::addStockPile(std::vector<ItemQuery>& queries)
{
	StockPile& stockPile = m_stockPiles.emplace_back(queries, m_area, m_faction);
	// Do not mark avalible yet, wait for the first open block to be added.
	return stockPile;
}
AreaHasStockPilesForFaction::AreaHasStockPilesForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& a, const FactionId& f) :
	m_area(a), m_faction(f)
{
	Items& items = m_area.getItems();
	if(data.contains("stockpiles"))
		for(const Json& stockPileData : data["stockpiles"])
			m_stockPiles.emplace_back(stockPileData, deserializationMemo, m_area);
	if(data.contains("availableStockPilesByItemType"))
		for(const Json& pair : data["availableStockPilesByItemType"])
		{
			ItemTypeId itemType = ItemType::byName(pair[0].get<std::wstring>());
			for(const Json& stockPileAddress : pair[1])
			{
				StockPile& stockPile = *deserializationMemo.m_stockpiles.at(stockPileAddress.get<uintptr_t>());
				m_availableStockPilesByItemType.getOrCreate(itemType).insert(&stockPile);
			}
		}
	if(data.contains("itemsWithoutDestinationsByItemType"))
		for(const Json& pair : data["itemsWithoutDestinationsByItemType"])
		{
			ItemTypeId itemType = ItemType::byName(pair[0].get<std::wstring>());
			for(const Json& item : pair[1])
			{
				ItemReference ref(item, items.m_referenceData);
				m_itemsWithoutDestinationsByItemType[itemType].insert(ref);
			}
		}
	if(data.contains("itemsToBeStockPiled"))
		for(const Json& item : data["itemsToBeStockPiled"])
		{
			ItemReference ref(item, items.m_referenceData);
			m_itemsToBeStockPiled.insert(ref);
		}
	if(data.contains("itemsWithDestinationsByStockPile"))
		for(const Json& pair : data["itemsWithDestinationsByStockPile"])
		{
			StockPile& stockPile = *deserializationMemo.m_stockpiles.at(pair[0].get<uintptr_t>());
			for(const Json& item : pair[1])
			{
				ItemReference ref(item, items.m_referenceData);
				m_itemsWithDestinationsByStockPile.getOrCreate(&stockPile).insert(ref);
			}
		}
	if(data.contains("projectsByItem"))
		for(const Json& pair : data["projectsByItem"])
		{
			ItemReference ref(pair[0], items.m_referenceData);
			for(const Json& projectData : pair[1])
			{
				StockPileProject& project = m_projects.emplace_back(projectData, deserializationMemo, m_area);
				m_projectsByItem.getOrCreate(ref).insert(&project);
			}
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
			Json jsonPair{ItemType::getName(pair.first), Json::array()};
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
			Json jsonPair{ItemType::getName(pair.first), Json::array()};
			for(ItemReference item : pair.second)
				jsonPair[1].push_back(item);
			data["itemsWithoutDestinationsByItemType"].push_back(jsonPair);
		}
	}
	if(!m_itemsToBeStockPiled.empty())
	{
		data["itemsToBeStockPiled"] = Json::array();
		for(ItemReference item : m_itemsToBeStockPiled)
			data["itemsToBeStockPiled"].push_back(item);
	}
	if(!m_itemsWithDestinationsByStockPile.empty())
	{
		data["itemsWithDestinationsByStockPile"] = Json::array();
		for(auto& pair : m_itemsWithDestinationsByStockPile)
		{
			Json jsonPair{reinterpret_cast<uintptr_t>(pair.first), Json::array()};
			for(ItemReference item : pair.second)
				jsonPair[1].push_back(item);
			data["itemsWithDestinationsByStockPile"].push_back(jsonPair);
		}
	}
	if(!m_projectsByItem.empty())
	{
		data["projectsByItem"] = Json::array();
		for(auto& pair : m_projectsByItem)
		{
			Json jsonPair{pair.first, Json::array()};
			for(const StockPileProject* stockPileProject : pair.second)
				jsonPair[1].push_back(stockPileProject->toJson());
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
	{
		auto found = m_availableStockPilesByItemType.find(itemQuery.m_itemType);
		if(found != m_availableStockPilesByItemType.end())
			found->second.maybeErase(&stockPile);
	}
	Items& items = m_area.getItems();
	// Potentially transfer items from withDestination to withoutDestination, also cancel any associated projects and remove from items with destinations without projects.
	// Sort items which no longer have a potential destination from ones which have a new one.
	if(m_itemsWithDestinationsByStockPile.contains(&stockPile))
		for(ItemReference item : m_itemsWithDestinationsByStockPile[&stockPile])
		{
			ItemIndex itemIndex = item.getIndex(items.m_referenceData);
			StockPile* newStockPile = getStockPileFor(itemIndex);
			assert(newStockPile != &stockPile);
			if(newStockPile == nullptr)
			{
				// Remove all those which no longer have a destination from m_itemsToBeStockPiled.
				// Add them to items without destinaitons by item type.
				auto found = m_itemsToBeStockPiled.find(item);
				if(found != m_itemsToBeStockPiled.end())
				{
					m_itemsToBeStockPiled.erase(found);
					m_itemsWithoutDestinationsByItemType[items.getItemType(itemIndex)].insert(item);
				}
			}
			else
				// Record the new stockpile in items with destinations by stockpile.
				m_itemsWithDestinationsByStockPile[newStockPile].insert(item);
		}
	// Remove the stockpile entry from items with destinations by stockpile.
	m_itemsWithDestinationsByStockPile.maybeErase(&stockPile);
	// Destruct.
	m_stockPiles.remove(stockPile);
}
bool AreaHasStockPilesForFaction::isValidStockPileDestinationFor(const BlockIndex& block, const ItemIndex& item) const
{
	Blocks& blocks = m_area.getBlocks();
	if(blocks.isReserved(block, m_faction))
		return false;
	if(!blocks.stockpile_isAvalible(block, m_faction))
		return false;
	return blocks.stockpile_getForFaction(block, m_faction)->accepts(item);
}
void AreaHasStockPilesForFaction::addItem(const ItemIndex& item)
{
	Items& items = m_area.getItems();
	items.stockpile_set(item, m_faction);
	// This isn't neccessarily the stockpile that the item is going to, it is only the one that proved that there exists a stockpile somewhere which accepts this item.
	StockPile* stockPile = getStockPileFor(item);
	ItemReference ref = items.getReference(item);
	if(stockPile == nullptr)
		m_itemsWithoutDestinationsByItemType.getOrCreate(items.getItemType(item)).insert(ref);
	else
	{
		m_itemsWithDestinationsByStockPile.getOrCreate(stockPile).insert(ref);
		m_itemsToBeStockPiled.insert(ref);
	}
	AreaHasBlockDesignationsForFaction& hasDesignations = m_area.m_blockDesignations.getForFaction(m_faction);
	for(const BlockIndex& block : items.getBlocks(item))
		hasDesignations.maybeSet(block, BlockDesignation::StockPileHaulFrom);

}
void AreaHasStockPilesForFaction::maybeAddItem(const ItemIndex& item)
{
	Items& items = m_area.getItems();
	ItemTypeId itemType = items.getItemType(item);
	ItemReference ref = items.getReference(item);
	if(!m_projectsByItem.contains(ref) &&
		(
			!m_itemsWithoutDestinationsByItemType.contains(itemType) ||
			!m_itemsWithoutDestinationsByItemType[itemType].contains(ref)
		)
	)
		addItem(item);
}
void AreaHasStockPilesForFaction::removeItem(const ItemIndex& item)
{
	Items& items = m_area.getItems();
	ItemTypeId itemType = m_area.getItems().getItemType(item);
	items.stockpile_maybeUnset(item, m_faction);
	ItemReference ref = items.getReference(item);
	if(m_itemsWithoutDestinationsByItemType.contains(itemType) && m_itemsWithoutDestinationsByItemType[itemType].contains(ref))
		m_itemsWithoutDestinationsByItemType[itemType].erase(ref);
	else
	{
		m_itemsToBeStockPiled.maybeErase(ref);
		m_itemsWithDestinationsByStockPile.eraseIf([&](auto& pair){
			pair.second.maybeErase(ref);
			return pair.second.empty();
		});
	}
	auto found = m_projectsByItem.find(ref);
	if(found != m_projectsByItem.end())
	{
		for (StockPileProject* stockPileProject : found->second)
		{
			stockPileProject->cancel();
			m_projects.remove(*stockPileProject);
		}
	}
	// Unset block designation for all occupied unless the block contains another item which is also set to be stockpiled by the same faction.
	AreaHasBlockDesignationsForFaction& hasDesignations = m_area.m_blockDesignations.getForFaction(m_faction);
	Blocks& blocks = m_area.getBlocks();
	for(const BlockIndex& block : items.getBlocks(item))
	{
		for(const ItemIndex& item : blocks.item_getAll(block))
			if(items.stockpile_canBeStockPiled(item, m_faction))
				return;
		hasDesignations.maybeUnset(block, BlockDesignation::StockPileHaulFrom);
	}
}
void AreaHasStockPilesForFaction::maybeRemoveFromItemsWithDestinationByStockPile(const StockPile& stockPile, const ItemIndex& item)
{
	Items& items = m_area.getItems();
	StockPile* nonConst = const_cast<StockPile*>(&stockPile);
	if(m_itemsWithDestinationsByStockPile[nonConst].size() == 1 && m_itemsWithDestinationsByStockPile[nonConst].front().getIndex(items.m_referenceData) == item)
		m_itemsWithDestinationsByStockPile.erase(nonConst);
	else
	{
		ItemReference ref = m_area.getItems().getReference(item);
		m_itemsWithDestinationsByStockPile[nonConst].maybeErase(ref);
	}
}
void AreaHasStockPilesForFaction::updateItemReferenceForProject(StockPileProject& project, const ItemReference& ref)
{
	removeFromProjectsByItem(project);
	m_projectsByItem.getOrCreate(ref).insert(&project);
}
void AreaHasStockPilesForFaction::removeBlock(const BlockIndex& block)
{
	StockPile* stockpile = m_area.getBlocks().stockpile_getForFaction(block, m_faction);
	if(stockpile != nullptr)
		stockpile->removeBlock(block);
}
void AreaHasStockPilesForFaction::setAvailable(StockPile& stockPile)
{
	assert(!m_itemsWithDestinationsByStockPile.contains(&stockPile));
	Items& items = m_area.getItems();
	for(ItemQuery& itemQuery : stockPile.m_queries)
	{
		assert(itemQuery.m_itemType.exists());
		m_availableStockPilesByItemType.getOrCreate(itemQuery.m_itemType).maybeInsert(&stockPile);
		auto found = m_itemsWithoutDestinationsByItemType.find(itemQuery.m_itemType);
		bool inserted = false;
		if(found != m_itemsWithoutDestinationsByItemType.end())
			for(ItemReference item : found->second)
				if(itemQuery.query(m_area, item.getIndex(items.m_referenceData)))
				{
					m_itemsWithDestinationsByStockPile.getOrCreate(&stockPile).insert(item);
					m_itemsToBeStockPiled.insert(item);
					inserted = true;
				}
		if(inserted)
			m_itemsWithoutDestinationsByItemType.erase(found);
	}
}
void AreaHasStockPilesForFaction::setUnavailable(StockPile& stockPile)
{
	for(ItemQuery& itemQuery : stockPile.m_queries)
		m_availableStockPilesByItemType[itemQuery.m_itemType].erase(&stockPile);
	if(m_itemsWithDestinationsByStockPile.contains(&stockPile))
	{
		Items& items = m_area.getItems();
		for(ItemReference item : m_itemsWithDestinationsByStockPile[&stockPile])
		{
			ItemIndex itemIndex = item.getIndex(items.m_referenceData);
			StockPile* newStockPile = getStockPileFor(itemIndex);
			if(newStockPile == nullptr)
				m_itemsWithoutDestinationsByItemType.getOrCreate(items.getItemType(itemIndex)).maybeInsert(item);
			else
				m_itemsWithDestinationsByStockPile.getOrCreate(newStockPile).maybeInsert(item);
		}
		m_itemsWithDestinationsByStockPile.erase(&stockPile);
	}
}
void AreaHasStockPilesForFaction::makeProject(const ItemIndex& item, const BlockIndex& destination, StockPileObjective& objective, const ActorIndex& actor)
{
	Blocks& blocks = m_area.getBlocks();
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	assert(blocks.stockpile_contains(destination, m_faction));
	// Quantity per project is the ammount that can be hauled by hand, if any, or one.
	// TODO: Hauling a load of generics with tools.
	Quantity quantity = std::min({
		items.getQuantity(item),
		actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(actor, items.getSingleUnitMass(item), Config::minimumHaulSpeedInital),
		blocks.shape_getQuantityOfItemWhichCouldFit(destination, items.getItemType(item))
	});
	// Max workers is one if at least one can be hauled by hand.
	Quantity maxWorkers = Quantity::create(1);
	if(quantity == 0)
	{
		quantity = Quantity::create(1);
		maxWorkers = Quantity::create(2);
	}
	ItemReference ref = m_area.getItems().getReference(item);
	StockPileProject& project = m_projects.emplace_back(m_faction, m_area, destination, item, quantity, maxWorkers);
	m_projectsByItem.getOrCreate(ref).insert(&project);
	std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<StockPileHasShapeDishonorCallback>(project);
	project.setLocationDishonorCallback(std::move(dishonorCallback));
	project.addWorkerCandidate(actor, objective);
	objective.m_project = &project;
	if(items.reservable_getUnreservedCount(item, m_faction) == quantity)
	{
		items.stockpile_maybeUnset(item, m_faction);
		m_itemsToBeStockPiled.maybeErase(ref);
	}
}
void AreaHasStockPilesForFaction::cancelProject(StockPileProject& project)
{
	Items& items = m_area.getItems();
	ItemIndex item = project.m_item.getIndex(items.m_referenceData);
	destroyProject(project);
	addItem(item);
}
void AreaHasStockPilesForFaction::destroyProject(StockPileProject& project)
{
	removeFromProjectsByItem(project);
	m_projects.remove(project);
}
void AreaHasStockPilesForFaction::removeFromProjectsByItem(StockPileProject& project)
{
	if(m_projectsByItem[project.m_item].size() == 1)
		m_projectsByItem.erase(project.m_item);
	else
		m_projectsByItem[project.m_item].erase(&project);
}
void AreaHasStockPilesForFaction::addQuery(StockPile& stockPile, const ItemQuery& query)
{
	stockPile.addQuery(query);
	assert(query.m_itemType.exists());
	m_availableStockPilesByItemType.getOrCreate(query.m_itemType).insert(&stockPile);
}
void AreaHasStockPilesForFaction::removeQuery(StockPile& stockPile, const ItemQuery& query)
{
	stockPile.removeQuery(query);
	assert(query.m_itemType.exists());
	assert(m_availableStockPilesByItemType.contains(query.m_itemType));
	if(m_availableStockPilesByItemType[query.m_itemType].size() == 1)
		m_availableStockPilesByItemType.erase(query.m_itemType);
	else
		m_availableStockPilesByItemType[query.m_itemType].erase(&stockPile);
}
bool AreaHasStockPilesForFaction::isAnyHaulingAvailableFor(const ActorIndex& actor) const
{
	assert(m_faction == m_area.getActors().getFactionId(actor));
	return !m_itemsToBeStockPiled.empty();
}
ItemIndex AreaHasStockPilesForFaction::getHaulableItemForAt(const ActorIndex& actor, const BlockIndex& block)
{
	assert(m_area.getActors().getFaction(actor).exists());
	Blocks& blocks = m_area.getBlocks();
	FactionId faction = m_area.getActors().getFactionId(actor);
	Items& items = m_area.getItems();
	if(blocks.isReserved(block, faction))
		return ItemIndex::null();
	for(ItemIndex item : blocks.item_getAll(block))
	{
		if(items.reservable_isFullyReserved(item, faction))
			continue;
		if(items.stockpile_canBeStockPiled(item, faction))
			return item;
	}
	return ItemIndex::null();
}
StockPile* AreaHasStockPilesForFaction::getStockPileFor(const ItemIndex& item) const
{
	ItemTypeId itemType = m_area.getItems().getItemType(item);
	if(!m_availableStockPilesByItemType.contains(itemType))
		return nullptr;
	for(StockPile* stockPile : m_availableStockPilesByItemType[itemType])
		if(stockPile->accepts(item))
			return stockPile;
	return nullptr;
}
AreaHasStockPilesForFaction& AreaHasStockPiles::getForFaction(const FactionId& faction)
{
	if(!m_data.contains(faction))
		registerFaction(faction);
	return m_data[faction];
}
void AreaHasStockPiles::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, m_area, faction);
	}
}
void AreaHasStockPiles::loadWorkers(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data[faction].loadWorkers(pair[1], deserializationMemo);
	}
}
void AreaHasStockPiles::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair : pair.second->m_projectsByItem)
			for(auto& project : pair.second)
				project->clearReservations();
}
Json AreaHasStockPiles::toJson() const
{
	Json data = Json::array();
	for(auto& pair : m_data)
	{
		Json jsonPair{pair.first, pair.second->toJson()};
		data.push_back(jsonPair);
	}
	return data;
}