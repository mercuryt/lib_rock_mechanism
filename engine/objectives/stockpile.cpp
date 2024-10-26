#include "stockpile.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../blocks/blocks.h"
#include "../terrainFacade.h"
#include "types.h"
// Searches for an Item and destination to make a hauling project for m_objective.m_actor.
StockPilePathRequest::StockPilePathRequest(Area& area, StockPileObjective& spo, const ActorIndex& actor) : m_objective(spo)
{
	assert(m_objective.m_project == nullptr);
	assert(!m_objective.hasDestination());
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	assert(!actors.project_exists(actor));
	FactionId faction = actors.getFactionId(actor);
	if(!m_objective.m_item.exists())
	{
		auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
		std::function<bool(const BlockIndex&)> blocksContainsItemCondition = [this, &hasStockPiles, &items, actor](const BlockIndex& block)
		{
			ItemIndex item = hasStockPiles.getHaulableItemForAt(actor, block);
			if(item.empty())
				return false;
			std::tuple<ItemTypeId, MaterialTypeId> tuple = {items.getItemType(item), items.getMaterialType(item)};
			return std::ranges::find(m_objective.m_closedList, tuple) == m_objective.m_closedList.end();
		};
		bool unreserved = false;
		DistanceInBlocks maxRange = Config::maxRangeToSearchForStockPileItems;
		createGoAdjacentToCondition(area, actor, blocksContainsItemCondition, m_objective.m_detour, unreserved, maxRange, BlockIndex::null());
	}
	else
	{
		std::function<bool(const BlockIndex&)> condition = [this, &area, actor](const BlockIndex& block) { return m_objective.destinationCondition(area, block, m_objective.m_item.getIndex(), actor); };
		bool unreserved = false;
		DistanceInBlocks maxRange = Config::maxRangeToSearchForStockPiles;
		createGoAdjacentToCondition(area, actor, condition, m_objective.m_detour, unreserved, maxRange, BlockIndex::null());
	}
}
void StockPilePathRequest::callback(Area& area, const FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	FactionId faction = actors.getFactionId(actor);
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	if(!m_objective.m_item.exists())
	{
		if(result.path.empty() && !result.useCurrentPosition)
			// No haulable item found.
			actors.objective_canNotCompleteObjective(actor, m_objective);
		else
		{
			ItemIndex item = hasStockPiles.getHaulableItemForAt(actor, result.blockThatPassedPredicate);
			if(item.exists())
				m_objective.m_item.setTarget(area.getItems().getReferenceTarget(item));
			// Reenter with item set.
			callback(area, result);
		}
	}
	else
	{
		if(result.path.empty() && !result.useCurrentPosition)
		{
			// No stockpile found.
			Items& items = area.getItems();
			const ItemIndex& itemIndex = m_objective.m_item.getIndex();
			m_objective.m_closedList.emplace_back(items.getItemType(itemIndex), items.getMaterialType(itemIndex));
			m_objective.execute(area, actor);
		}
		else
		{
			if(!m_objective.destinationCondition(area, result.blockThatPassedPredicate, m_objective.m_item.getIndex(), actor))
				// Found destination is no longer valid.
				m_objective.execute(area, actor);
			else
			{
				// Destination found, join or create a project.
				assert(m_objective.m_stockPileLocation.exists());
				FactionId faction = actors.getFactionId(actor);
				Blocks& blocks = area.getBlocks();
				assert(blocks.stockpile_getForFaction(m_objective.m_stockPileLocation, faction));
				StockPile& stockpile = *blocks.stockpile_getForFaction(m_objective.m_stockPileLocation, faction);
				if(stockpile.hasProjectNeedingMoreWorkers())
					stockpile.addToProjectNeedingMoreWorkers(actor, m_objective);
				else
				{
					auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
					if(hasStockPiles.m_projectsByItem.contains(m_objective.m_item))
					{
						// Projects found, select one to join.
						for(StockPileProject& stockPileProject : hasStockPiles.m_projectsByItem.at(m_objective.m_item))
							if(stockPileProject.canAddWorker(actor))
							{
								m_objective.m_project = &stockPileProject;
								stockPileProject.addWorkerCandidate(actor, m_objective);
							}
					}
					else
						// No projects found, make one.
						hasStockPiles.makeProject(m_objective.m_item.getIndex(), m_objective.m_stockPileLocation, m_objective, actor);
				}
			}
		}
	}
}
StockPilePathRequest::StockPilePathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	m_objective(static_cast<StockPileObjective&>(*deserializationMemo.m_objectives[data["objective"]]))
{
	nlohmann::from_json(data, static_cast<PathRequest&>(*this));
}
Json StockPilePathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = reinterpret_cast<uintptr_t>(&m_objective);
	return output;
}
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
	m_project(data.contains("project") ? static_cast<StockPileProject*>(deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) : nullptr) { }
Json StockPileObjective::toJson() const
{
	Json data = Objective::toJson();
	if(m_project != nullptr)
		data["project"] = *m_project;
	return data;
}
void StockPileObjective::execute(Area& area, const ActorIndex& actor) 
{ 
	// If there is no project to work on dispatch a threaded task to either find one or call cannotFulfillObjective.
	if(m_project == nullptr)
	{
		Actors& actors = area.getActors();
		assert(!actors.project_exists(actor));
		actors.move_pathRequestRecord(actor, std::make_unique<StockPilePathRequest>(area, *this, actor));
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
		actors.project_unset(actor);
	}
	else
		assert(!actors.project_exists(actor));
	actors.move_pathRequestMaybeCancel(actor);
	actors.canReserve_clearAll(actor);
}
bool StockPileObjective::destinationCondition(Area& area, const BlockIndex& block, const ItemIndex& item, const ActorIndex& actor)
{
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	assert(m_stockPileLocation.empty());
	if(!blocks.shape_staticCanEnterCurrentlyWithAnyFacing(block, items.getShape(item), items.getBlocks(item)))
		return false;
	if(!blocks.item_empty(block))
		// Don't put multiple items in the same block unless they are generic and share item type and material type.
		if(!items.isGeneric(item) || blocks.item_getCount(block, items.getItemType(item), items.getMaterialType(item)).empty())
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
