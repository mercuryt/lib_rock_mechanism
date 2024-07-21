#include "unequipItem.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "items/items.h"

UnequipItemObjective::UnequipItemObjective(ItemReference item, BlockIndex block) : 
	Objective(Config::equipPriority), m_item(item), m_block(block) { }
UnequipItemObjective::UnequipItemObjective(const Json& data, Area& area) :
	Objective(data), 
	m_block(data["block"].get<BlockIndex>())
{
	m_item.load(data["item"], area);
}
void UnequipItemObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	if(!actors.isAdjacentToLocation(actor, m_block))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToLocation(actor, m_block, false, false, false);
	else
	{
		Blocks& blocks = area.getBlocks();
		Items& items = area.getItems();
		ItemIndex item = m_item.getIndex();
		if(blocks.shape_canEnterCurrentlyWithAnyFacing(m_block, items.getShape(item), {}))
		{
			actors.equipment_remove(actor, item);
			items.setLocation(item, m_block);
			actors.objective_complete(actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(actor, *this);
	}
}
void UnequipItemObjective::cancel(Area& area, ActorIndex actor) { area.getActors().canReserve_clearAll(actor); }
void UnequipItemObjective::reset(Area& area, ActorIndex actor) { cancel(area, actor); }
