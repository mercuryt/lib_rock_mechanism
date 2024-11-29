#include "unequipItem.h"
#include "../area.h"
#include "../deserializationMemo.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "items/items.h"

UnequipItemObjective::UnequipItemObjective(const ItemReference& item, const BlockIndex& block) : 
	Objective(Config::equipPriority), m_item(item), m_block(block) { }
UnequipItemObjective::UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), 
	m_block(data["block"].get<BlockIndex>())
{
	data["item"].get_to(m_item);
}
void UnequipItemObjective::execute(Area& area, const ActorIndex& actor)
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
		ItemIndex item = m_item.getIndex(items.m_referenceData);
		ShapeId shape = items.getShape(item);
		const BlockIndices& occupied = items.getBlocks(item);
		if(blocks.shape_canEnterCurrentlyWithAnyFacing(m_block, shape, occupied))
		{
			actors.equipment_remove(actor, item);
			items.setLocationAndFacing(item, m_block, Facing::create(0));
			actors.objective_complete(actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(actor, *this);
	}
}
void UnequipItemObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().canReserve_clearAll(actor); }
void UnequipItemObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
Json UnequipItemObjective::toJson() const
{
	Json output = static_cast<const Objective&>(*this).toJson();
	output.update({
		{"item", m_item},
		{"block", m_block}
	});
	return output;
}
