#include "stockpile.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
#include "../terrainFacade.h"
#include "types.h"
// Objective Type.
bool StockPileObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	return area.m_hasStockPiles.getForFaction(area.getActors().getFactionId(actor)).isAnyHaulingAvailableFor(actor);
}
std::unique_ptr<Objective> StockPileObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	return std::make_unique<StockPileObjective>();
}
// Objective.
StockPileObjective::StockPileObjective() : Objective(Config::stockPilePriority) { }
StockPileObjective::StockPileObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo),
	m_project(data.contains("project") ? static_cast<StockPileProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr)
{ 
	if(data.contains("item"))
		data["item"].get_to(m_item);
	if(data.contains("stockPileLocation"))
		m_stockPileLocation = data["stockPileLocation"].get<BlockIndex>();
	if(data.contains("hasCheckedForDropOffLocation"))
		m_hasCheckedForCloserDropOffLocation = true;
	if(data.contains("pickUpFacing"))
	{
		m_pickUpFacing = data["pickUpFacing"].get<Facing>();
		m_pickUpLocation = data["pickUpLocation"].get<BlockIndex>();
	}
}
Json StockPileObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = *m_project;
	if(m_item.exists())
		data["item"] = m_item;
	if(m_stockPileLocation.exists())
		data["stockPileLocation"] = m_stockPileLocation;
	if(m_hasCheckedForCloserDropOffLocation)
		data["hasCheckedForDropOffLocation"] = true;
	if(m_pickUpFacing.exists())
	{
		assert(m_pickUpLocation.exists());
		data["m_pickUpFacing"] = m_pickUpFacing;
		data["m_pickUpLocation"] = m_pickUpLocation;
	}
	return data;
}
void StockPileObjective::execute(Area& area, const ActorIndex& actor) 
{ 
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
	{
		Actors& actors = area.getActors();
		assert(!actors.project_exists(actor));
		if(m_item.empty())
			actors.move_pathRequestRecord(actor, std::make_unique<StockPilePathRequest>(area, *this, actor));
		else
			actors.move_pathRequestRecord(actor, std::make_unique<StockPileDestinationPathRequest>(area, *this, actor));
	}
	else
		m_project->commandWorker(actor);
}
void StockPileObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(m_project != nullptr)
	{
		m_project->removeWorker(actor);
		m_project = nullptr;
	}
	else
		assert(!actors.project_exists(actor));
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
void StockPileObjective::reset(Area& area, const ActorIndex& actor)
{
	m_hasCheckedForCloserDropOffLocation = false;
	m_item.clear();
	m_stockPileLocation.clear();
	if constexpr(DEBUG)
	{
		m_pickUpFacing.clear();
		m_pickUpLocation.clear();
	}
	cancel(area, actor);
}
bool StockPileObjective::destinationCondition(Area& area, const BlockIndex& block, const ItemIndex& item, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	if(!blocks.shape_staticCanEnterCurrentlyWithAnyFacing(block, items.getShape(item), items.getBlocks(item)))
		return false;
	if(!blocks.item_empty(block))
		// Don't put multiple items in the same block unless they are generic and share item type and material type.
		if(!items.isGeneric(item) || blocks.item_getAll(block).size() != 1 || blocks.item_getCount(block, items.getItemType(item), items.getMaterialType(item)) == 0)
			return false;
	FactionId faction = actors.getFactionId(actor);
	const StockPile* stockpile = blocks.stockpile_getForFaction(block, faction);
	if(stockpile == nullptr || !stockpile->isEnabled() || !blocks.stockpile_isAvalible(block, faction))
		return false;
	if(blocks.isReserved(block, faction))
		return false;
	if(!stockpile->accepts(item))
		return false;
	m_stockPileLocation = block;
	return true;
};
// Path Reqests
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
StockPilePathRequest::StockPilePathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actor) : m_objective(spo)
{
	assert(m_objective.m_project == nullptr);
	assert(!m_objective.hasDestination());
	assert(!m_objective.m_hasCheckedForCloserDropOffLocation);
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	assert(!actors.project_exists(actor));
	const FactionId& faction = actors.getFactionId(actor);
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	std::function<bool(const BlockIndex&)> condition = [this, actor, faction, &hasStockPiles, &blocks, &items, &area](const BlockIndex& block){
		for(ItemIndex item : blocks.item_getAll(block))
		{
			// Multi block items could be encountered more then once.
			if(m_items.contains(item))
				continue;
			if(items.reservable_isFullyReserved(item, faction))
				// Cannot stockpile reserved items.
				continue;
			if(items.stockpile_canBeStockPiled(item, faction))
			{
				// Item can be stockpiled, check if any of the stockpiles we have seen so far will accept it.
				for (const auto& [stockpile, blocks] : m_blocksByStockPile)
					for(const BlockIndex& block : blocks)
						if (stockpile->accepts(item) && checkDestination(area, item, block))
						{
							// Success
							m_objective.m_item = area.getItems().m_referenceData.getReference(item);
							m_objective.m_stockPileLocation = block;
							return true;
						}
				// Item does not match any stockpile seen so far, store it to compare against future stockpiles.
				m_items.insert(item);
			}
		}
		StockPile *stockPile = blocks.stockpile_getForFaction(block, faction);
		if(stockPile != nullptr && (!m_blocksByStockPile.contains(stockPile) || !m_blocksByStockPile[stockPile].contains(block)))
		{
			if(blocks.isReserved(block, faction))
				return false;
			if(!blocks.item_empty(block))
			{
				// Block static volume is full, don't stockpile here.
				if(blocks.shape_getStaticVolume(block) == Config::maxBlockVolume)
					return false;
				// There is more then one item type in this block, don't stockpile anything here.
				if(blocks.item_getAll(block).size() != 1)
					return false;
				// Non generics don't stack, don't stockpile anything here if there is a non generic present.
				ItemTypeId itemType = items.getItemType(blocks.item_getAll(block).front());
				if(!ItemType::getIsGeneric(itemType))
					return false;
			}
			for(ItemIndex item : m_items)
				if(stockPile->accepts(item) && checkDestination(area, item, block))
				{
					// Success
					m_objective.m_item = area.getItems().m_referenceData.getReference(item);
					m_objective.m_stockPileLocation = block;
					return true;
				}
			// No item seen so far matches the found stockpile, store it to check against future items.
			m_blocksByStockPile.getOrCreate(stockPile).add(block);
		}
		return false;
	};
	bool unreserved = false;
	//TODO: Optimization oppurtunity: path is unused.
	createGoAdjacentToCondition(area, actor, condition, m_objective.m_detour, unreserved, Config::maxRangeToSearchForStockPileItems, BlockIndex::null());
}
void StockPilePathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	const ActorIndex& actor = getActor();
	const FactionId& faction = actors.getFactionId(actor);
	if(!m_objective.m_item.exists())
	{
		// No haulable item found.
		actors.objective_canNotCompleteObjective(actor, m_objective);
	}
	else
	{
		assert(m_objective.m_stockPileLocation.exists());
		Items& items = area.getItems();
		const ItemIndex& item = m_objective.m_item.getIndex(items.m_referenceData);
		if(
			items.reservable_isFullyReserved(item, faction) ||
			!m_objective.destinationCondition(area, m_objective.m_stockPileLocation, item, actor)
		)
		{
			// Found destination is no longer valid or item has been reserved
			m_objective.m_item.clear();
			m_objective.m_stockPileLocation.clear();
			m_objective.execute(area, actor);
		}
		else
		{
			// Haulable Item and potential destination found. Call StockPileObjective::execute to create the destination path request in order to try and find a better destination.
			Blocks& blocks = area.getBlocks();
			m_objective.m_pickUpLocation = result.path.back();
			if(result.useCurrentPosition)
				m_objective.m_pickUpFacing = actors.getFacing(actor);
			else
			{
				const BlockIndex &secondToLast = result.path.size() > 1 ? *(result.path.end() - 2) : actors.getLocation(actor);
				m_objective.m_pickUpFacing = blocks.facingToSetWhenEnteringFrom(result.path.back(), secondToLast);
			}
			m_objective.execute(area, actor);
		}
	}
}
bool StockPilePathRequest::checkDestination(const Area& area, const ItemIndex& item, const BlockIndex& block) const
{
	const Blocks& blocks = area.getBlocks();
	const Items& items = area.getItems();
	// Only stockpile in non empty block if it means stacking generics.
	if(!blocks.item_empty(block))
	{
		const ItemTypeId& itemType = items.getItemType(item);
		if(!ItemType::getIsGeneric(itemType))
			return false;
		// item cannot stack with the item in this block, don't stockpile it here.
		if(blocks.item_getCount(block, itemType, items.getMaterialType(item)) == 0)
			return false;
	}
	if(!blocks.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithAnyFacing(block, items.getShape(item), items.getMoveType(item), {}))
		// This item can't fit here.
		return false;
	return true;
}
StockPilePathRequest::StockPilePathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json StockPilePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	output["type"] = "stockpile";
	return output;
}
StockPileDestinationPathRequest::StockPileDestinationPathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actor) : m_objective(spo)
{
	assert(m_objective.m_project == nullptr);
	// A destination exists but we want to search for a better one.
	assert(m_objective.hasDestination());
	assert(m_objective.hasItem());
	assert(!m_objective.m_hasCheckedForCloserDropOffLocation);
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	assert(!actors.project_exists(actor));
	const FactionId& faction = actors.getFactionId(actor);
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	std::function<bool(const BlockIndex&)> condition = [this, actor, faction, &hasStockPiles, &blocks, &items, &area](const BlockIndex& block){
		return m_objective.destinationCondition(area, block, m_objective.m_item.getIndex(items.m_referenceData), actor);
	};
	const bool unreserved = false;
	createGoAdjacentToConditionFrom(area, actor, m_objective.m_pickUpLocation, m_objective.m_pickUpFacing, condition, m_objective.m_detour, unreserved, DistanceInBlocks::max(), BlockIndex::null());
}
void StockPileDestinationPathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		// Previously found path is no longer avaliable.
		actors.objective_canNotCompleteSubobjective(getActor());
		return;
	}
	m_objective.m_stockPileLocation = result.blockThatPassedPredicate;
	m_objective.m_hasCheckedForCloserDropOffLocation = true;
	// Destination found, join or create a project.
	assert(m_objective.m_stockPileLocation.exists());
	Blocks &blocks = area.getBlocks();
	const ActorIndex& actor = getActor();
	actors.canReserve_clearAll(actor);
	const FactionId& faction = actors.getFactionId(actor);
	assert(blocks.stockpile_getForFaction(m_objective.m_stockPileLocation, faction));
	StockPile &stockpile = *blocks.stockpile_getForFaction(m_objective.m_stockPileLocation, faction);
	if (stockpile.hasProjectNeedingMoreWorkers())
		stockpile.addToProjectNeedingMoreWorkers(actor, m_objective);
	else
	{
		auto &hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
		if (hasStockPiles.m_projectsByItem.contains(m_objective.m_item))
		{
			// Projects found, select one to join.
			for (StockPileProject& stockPileProject : hasStockPiles.m_projectsByItem[m_objective.m_item])
				if (stockPileProject.canAddWorker(actor))
				{
					m_objective.m_project = &stockPileProject;
					stockPileProject.addWorkerCandidate(actor, m_objective);
					return;
				}
		}
		// No projects found, make one.
		Items& items = area.getItems();
		hasStockPiles.makeProject(m_objective.m_item.getIndex(items.m_referenceData), m_objective.m_stockPileLocation, m_objective, actor);
	}
}
StockPileDestinationPathRequest::StockPileDestinationPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]])) { }
Json StockPileDestinationPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}