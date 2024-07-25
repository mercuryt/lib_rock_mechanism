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
	ActorIndex actor = getActor();
	createGoAdjacentToCondition(area, actor, predicate, m_objective.m_detour, unreserved, Config::maxRangeToSearchForUniformEquipment, BlockIndex::null());
}
void UniformPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(!result.path.empty())
	{
		if(!actors.move_tryToReserveProposedDestination(actor, result.path))
		{
			// Cannot reserve location, try again.
			m_objective.reset(area, actor);
			m_objective.execute(area, actor);
		}
		else
		{
			BlockIndex block = result.blockThatPassedPredicate;
			if(!m_objective.blockContainsItem(area, block))
			{
				// Destination no longer suitable.
				m_objective.reset(area, actor);
				m_objective.execute(area, actor);
			}
			else
			{
				ItemIndex item = m_objective.getItemAtBlock(area, block);
				m_objective.select(area, item);
				actors.move_setPath(actor, result.path);
			}
		}
	}
	else 
	{
		if(result.useCurrentPosition)
		{
			ItemIndex item = m_objective.getItemAtBlock(area, result.blockThatPassedPredicate);
			if(item.empty())
			{
				m_objective.reset(area, actor);
				m_objective.execute(area, actor);
			}
			else
			{
				m_objective.select(area, item);
				m_objective.execute(area, actor);
			}
		}
		else
			actors.objective_canNotCompleteObjective(actor, m_objective);
	}
}
// UniformObjective
UniformObjective::UniformObjective(Area& area, ActorIndex actor) :
	Objective(Config::equipPriority), m_elementsCopy(area.getActors().uniform_get(actor).elements)
{ 
	assert(area.getActors().uniform_exists(actor)); 
}
UniformObjective::UniformObjective(const Json& data, Area& area, ActorIndex actor) :
	Objective(data)
{ 
	area.getActors().m_hasUniform.at(actor)->recordObjective(*this);
}
Json UniformObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	return data;
}
void UniformObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(m_item.getIndex().exists())
	{
		if(!actors.isAdjacentToItem(actor, m_item.getIndex()))
			actors.move_setDestinationAdjacentToItem(actor, m_item.getIndex());
		else
		{
			if(actors.equipment_canEquipCurrently(actor, m_item.getIndex()))
				equip(area, m_item.getIndex(), actor);
			else
			{
				reset(area, actor);
				execute(area, actor);
			}
		}
	}		
	else
	{
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block){ return blockContainsItem(area, block); };
		BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(adjacent.exists())
			equip(area, getItemAtBlock(area, adjacent), actor);
		else
			actors.move_pathRequestRecord(actor, std::make_unique<UniformPathRequest>(area, *this));
	}
}
void UniformObjective::cancel(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
}
void UniformObjective::reset(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_item.clear();
}
ItemIndex UniformObjective::getItemAtBlock(Area& area, BlockIndex block)
{
	for(ItemIndex item : area.getBlocks().item_getAll(block))
		for(auto& element : m_elementsCopy)
			if(element.itemQuery.query(area, item))
				return item;
	return ItemIndex::null();
}
void UniformObjective::select(Area& area, ItemIndex item) { m_item.setTarget(area.getItems().getReferenceTarget(item)); }
void UniformObjective::equip(Area& area, ItemIndex item, ActorIndex actor)
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
	actors.equipment_add(actor, item);
	if(m_elementsCopy.empty())
		actors.objective_complete(actor, *this);
	else
	{
		reset(area, actor);
		execute(area, actor);
	}
}
