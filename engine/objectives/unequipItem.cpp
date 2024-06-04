#include "unequipItem.h"
#include "../actor.h"
#include "../area.h"
#include "../item.h"
#include "../deserializationMemo.h"

UnequipItemObjective::UnequipItemObjective(Actor& actor, Item& item, BlockIndex block) : 
	Objective(actor, Config::equipPriority), m_item(item), m_block(block) { }

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
void UnequipItemObjective::execute()
{
	if(!m_actor.isAdjacentTo(m_block))
		// detour, unresered, reserve.
		// TODO: detour.
		m_actor.m_canMove.setDestinationAdjacentTo(m_block, false, false, false);
	else
	{
		Blocks& blocks = m_actor.m_area->getBlocks();
		if(blocks.shape_canEnterCurrentlyWithAnyFacing(m_block, m_item))
		{
			m_actor.m_equipmentSet.removeEquipment(m_item);
			m_item.setLocation(m_block);
			m_actor.m_hasObjectives.objectiveComplete(*this);
		}
		else
			m_actor.m_hasObjectives.cannotFulfillObjective(*this);
	}
}
void UnequipItemObjective::cancel() { m_actor.m_canReserve.deleteAllWithoutCallback(); }
void UnequipItemObjective::reset() { cancel(); }
