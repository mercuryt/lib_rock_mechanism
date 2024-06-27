#include "equipItem.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../deserializationMemo.h"
#include "../area.h"
EquipItemObjective::EquipItemObjective(ActorIndex actor, ItemIndex item) : Objective(actor, Config::equipPriority), m_item(item) { }
/*
EquipItemObjective::EquipItemObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), m_item(deserializationMemo.itemReference(data["item"])) { }
Json EquipItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	return data;
}
*/
void EquipItemObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(!actors.isAdjacentToItem(m_actor, m_item))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToItem(m_actor, m_item, false, false, false);
	else
	{
		if(actors.getEquipmentSet(m_actor).canEquipCurrently(area, m_actor, m_item))
		{
			area.m_items.exit(m_item);
			actors.getEquipmentSet(m_actor).addEquipment(area, m_item);
			actors.objective_complete(m_actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(m_actor, *this);
	}
}
void EquipItemObjective::cancel(Area& area) { area.m_actors.canReserve_clearAll(m_actor); }
void EquipItemObjective::reset(Area& area) { cancel(area); }
