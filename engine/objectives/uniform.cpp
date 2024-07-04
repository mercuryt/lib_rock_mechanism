#include "uniform.h"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
#include "items/items.h"
#include "terrainFacade.h"
#include "types.h"
// Equip uniform.
UniformPathRequest::UniformPathRequest(Area& area, UniformObjective& objective) : m_objective(objective)
{
	std::function<bool(BlockIndex)> predicate = [&area, this](BlockIndex block){
		return m_objective.blockContainsItem(area, block);
	};
	bool unreserved = false;
	createGoAdjacentToCondition(area, m_objective.m_actor, predicate, m_objective.m_detour, unreserved, Config::maxRangeToSearchForUniformEquipment, BLOCK_INDEX_MAX);
}
void UniformPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	if(!result.path.empty())
	{
		if(!actors.move_tryToReserveProposedDestination(m_objective.m_actor, result.path))
		{
			// Cannot reserve location, try again.
			m_objective.reset(area);
			m_objective.execute(area);
		}
		else
		{
			BlockIndex block = result.blockThatPassedPredicate;
			if(!m_objective.blockContainsItem(area, block))
			{
				// Destination no longer suitable.
				m_objective.reset(area);
				m_objective.execute(area);
			}
			else
			{
				ItemIndex item = m_objective.getItemAtBlock(area, block);
				m_objective.select(item);
				actors.move_setPath(m_objective.m_actor, result.path);
			}
		}
	}
	else 
	{
		if(result.useCurrentPosition)
		{
			ItemIndex item = m_objective.getItemAtBlock(area, result.blockThatPassedPredicate);
			if(!item)
			{
				m_objective.reset(area);
				m_objective.execute(area);
			}
			else
			{
				m_objective.select(item);
				m_objective.execute(area);
			}
		}
		else
			actors.objective_canNotCompleteObjective(m_objective.m_actor, m_objective);
	}
}
// UniformObjective
UniformObjective::UniformObjective(Area& area, ActorIndex actor) :
	Objective(actor, Config::equipPriority), m_elementsCopy(area.getActors().uniform_get(actor).elements)
{ 
	assert(area.getActors().uniform_exists(actor)); 
}
/*
UniformObjective::UniformObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{ 
	m_actor.m_hasUniform.recordObjective(*this);
}
Json UniformObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
*/
void UniformObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(m_item)
	{
		if(!actors.isAdjacentToItem(m_actor, m_item))
			actors.move_setDestinationAdjacentToItem(m_actor, m_item);
		else
		{
			if(actors.equipment_canEquipCurrently(m_actor, m_item))
				equip(area, m_item);
			else
			{
				reset(area);
				execute(area);
			}
		}
	}		
	else
	{
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block){ return blockContainsItem(area, block); };
		BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(m_actor, predicate);
		if(adjacent != BLOCK_INDEX_MAX)
			equip(area, getItemAtBlock(area, adjacent));
		else
			actors.move_pathRequestRecord(m_actor, std::make_unique<UniformPathRequest>(area, *this));
	}
}
void UniformObjective::cancel(Area& area)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(m_actor);
}
void UniformObjective::reset(Area& area)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(m_actor);
	actors.move_pathRequestMaybeCancel(m_actor);
	m_item = ITEM_INDEX_MAX;
}
ItemIndex UniformObjective::getItemAtBlock(Area& area, BlockIndex block)
{
	for(ItemIndex item : area.getBlocks().item_getAll(block))
		for(auto& element : m_elementsCopy)
			if(element.itemQuery.query(area, item))
				return item;
	return ITEM_INDEX_MAX;
}
void UniformObjective::select(ItemIndex item) { m_item = item; }
void UniformObjective::equip(Area& area, ItemIndex item)
{
	Items& items = area.getItems();
	for(auto& element : m_elementsCopy)
		if(element.itemQuery.query(area, item))
		{
			if(items.getQuantity(item) >= element.quantity)
				std::erase(m_elementsCopy, element);
			else
				element.quantity -= items.getQuantity(item);
			break;
		}
	items.exit(item);
	Actors& actors = area.getActors();
	actors.equipment_add(m_actor, item);
	if(m_elementsCopy.empty())
		actors.objective_complete(m_actor, *this);
	else
	{
		reset(area);
		execute(area);
	}
}
