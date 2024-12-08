#include "uniform.h"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
#include "items/items.h"
#include "terrainFacade.h"
#include "types.h"
// Equip uniform.
UniformPathRequest::UniformPathRequest(Area& area, UniformObjective& objective, const ActorIndex& actor) : m_objective(objective)
{
	std::function<bool(BlockIndex)> predicate = [&area, this](const BlockIndex& block){
		return m_objective.blockContainsItem(area, block);
	};
	bool unreserved = false;
	createGoAdjacentToCondition(area, actor, predicate, m_objective.m_detour, unreserved, Config::maxRangeToSearchForUniformEquipment, BlockIndex::null());
}
UniformPathRequest::UniformPathRequest(const Json& data, DeserializationMemo& deserializationMemo) :
	PathRequest(data),
	m_objective(static_cast<UniformObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))) { }
void UniformPathRequest::callback(Area& area, const FindPathResult& result)
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
Json UniformPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = &m_objective;
	output["type"] = "uniform";
	return output;
}
// UniformObjective
UniformObjective::UniformObjective(Area& area, const ActorIndex& actor) :
	Objective(Config::equipPriority), m_elementsCopy(area.getActors().uniform_get(actor).elements)
{ 
	assert(area.getActors().uniform_exists(actor)); 
}
UniformObjective::UniformObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{ 
	area.getActors().m_hasUniform[actor]->recordObjective(*this);
}
Json UniformObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	return data;
}
void UniformObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	if(m_item.exists())
	{
		ItemIndex item = m_item.getIndex(items.m_referenceData);
		if(!actors.isAdjacentToItem(actor, item))
			actors.move_setDestinationAdjacentToItem(actor, item);
		else
		{
			if(actors.equipment_canEquipCurrently(actor, item))
				equip(area, item, actor);
			else
			{
				reset(area, actor);
				execute(area, actor);
			}
		}
	}		
	else
	{
		std::function<bool(const BlockIndex&)> predicate = [&](const BlockIndex& block){ return blockContainsItem(area, block); };
		BlockIndex adjacent = actors.getBlockWhichIsAdjacentWithPredicate(actor, predicate);
		if(adjacent.exists())
			equip(area, getItemAtBlock(area, adjacent), actor);
		else
			actors.move_pathRequestRecord(actor, std::make_unique<UniformPathRequest>(area, *this, actor));
	}
}
void UniformObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
}
void UniformObjective::reset(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_item.clear();
}
ItemIndex UniformObjective::getItemAtBlock(Area& area, const BlockIndex& block)
{
	Items& items = area.getItems();
	for(ItemIndex item : area.getBlocks().item_getAll(block))
		for(auto& element : m_elementsCopy)
			if(element.query(item, items))
				return item;
	return ItemIndex::null();
}
void UniformObjective::select(Area& area, const ItemIndex& item) { m_item.setIndex(item, area.getItems().m_referenceData); }
void UniformObjective::equip(Area& area, const ItemIndex& item, const ActorIndex& actor)
{
	Items& items = area.getItems();
	for(auto& element : m_elementsCopy)
		if(element.query(item, items))
		{
			if(items.getQuantity(item) >= element.m_quantity)
				std::erase(m_elementsCopy, element);
			else
				element.m_quantity -= items.getQuantity(item);
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