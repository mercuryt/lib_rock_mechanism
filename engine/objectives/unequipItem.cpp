#include "unequipItem.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "items/items.h"

UnequipItemObjective::UnequipItemObjective(ActorIndex actor, ItemIndex item, BlockIndex block) : 
	Objective(actor, Config::equipPriority), m_item(item), m_block(block) { }
/*
UnequipItemObjective::UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), 
	m_item(deserializationMemo.itemReference(data["item"])),
	m_block(data["block"].get<BlockIndex>()) { }
Json UnequipItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	data["block"] = m_block;
	return data;
}
*/
void UnequipItemObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(!actors.isAdjacentToLocation(m_actor, m_block))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToLocation(m_actor, m_block, false, false, false);
	else
	{
		Blocks& blocks = area.getBlocks();
		Items& items = area.getItems();
		if(blocks.shape_canEnterCurrentlyWithAnyFacing(m_block, items.getShape(m_item), {}))
		{
			actors.equipment_remove(m_actor, m_item);
			items.setLocation(m_item, m_block);
			actors.objective_complete(m_actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(m_actor, *this);
	}
}
void UnequipItemObjective::cancel(Area& area) { area.getActors().canReserve_clearAll(m_actor); }
void UnequipItemObjective::reset(Area& area) { cancel(area); }
