#include "unequipItem.h"
#include "../actor.h"
#include "../block.h"
UnequipItemObjective::UnequipItemObjective(Actor& actor, Item& item, Block& block) : Objective(actor, Config::equipPriority), m_item(item), m_block(block) { }

UnequipItemObjective::UnequipItemObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), 
	m_item(deserializationMemo.itemReference(data["item"])),
	m_block(deserializationMemo.blockReference(data["block"])) { }
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
		if(m_block.m_hasShapes.canEnterCurrentlyWithAnyFacing(m_item))
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
