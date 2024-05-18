#include "equipItem.h"
#include "../actor.h"
#include "../block.h"
#include "../item.h"
#include "../deserializationMemo.h"
EquipItemObjective::EquipItemObjective(Actor& actor, Item& item) : Objective(actor, Config::equipPriority), m_item(item) { }

EquipItemObjective::EquipItemObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_item(deserializationMemo.itemReference(data["item"])) { }
Json EquipItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	return data;
}
void EquipItemObjective::execute()
{
	if(!m_actor.isAdjacentTo(m_item))
		// detour, unresered, reserve.
		// TODO: detour.
		m_actor.m_canMove.setDestinationAdjacentTo(m_item, false, false, false);
	else
	{
		if(m_actor.m_equipmentSet.canEquipCurrently(m_item))
		{
			m_item.exit();
			m_actor.m_equipmentSet.addEquipment(m_item);
			m_actor.m_hasObjectives.objectiveComplete(*this);
		}
		else
			m_actor.m_hasObjectives.cannotFulfillObjective(*this);
	}
}
void EquipItemObjective::cancel() { m_actor.m_canReserve.deleteAllWithoutCallback(); }
void EquipItemObjective::reset() { cancel(); }
