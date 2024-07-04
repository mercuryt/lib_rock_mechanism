#include "stockpile.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
#include "../terrainFacade.h"
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
void StockPilePathRequest::callback(Area& area, FindPathResult& result)
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
