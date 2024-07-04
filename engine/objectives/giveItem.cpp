#include "giveItem.h"
#include "../actors/actors.h"
#include "../deserializationMemo.h"
#include "area.h"
#include "items/items.h"
GiveItemObjective::GiveItemObjective(Area& area, ActorIndex actor, ItemIndex item, ActorIndex recipient) :
	Objective(actor, Config::equipPriority), m_item(item), m_recipient(recipient) 
{ 
	createOnDestroyCallbacks(area);
}
/*
GiveItemObjective::GiveItemObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo), 
	m_item(deserializationMemo.itemReference(data["item"])),
	m_recipient(deserializationMemo.actorReference(data["recipent"])) 
{
	createOnDestroyCallbacks();
}
Json GiveItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	data["recipient"] = m_recipient;
	return data;
}
*/
void GiveItemObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(!actors.isAdjacentToActor(m_actor, m_recipient))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToActor(m_actor, m_recipient);
	else
	{
		if(actors.equipment_canEquipCurrently(m_recipient, m_item))
		{
			actors.equipment_remove(m_actor, m_item);
			actors.equipment_add(m_recipient, m_item);
			actors.objective_complete(m_actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(m_actor, *this);
	}
}
void GiveItemObjective::cancel(Area& area) { area.getActors().canReserve_clearAll(m_actor); }
void GiveItemObjective::reset(Area& area) { cancel(area); }
void GiveItemObjective::createOnDestroyCallbacks(Area& area) 
{ 
	std::function<void()> onDestory = [this, &area]{ cancel(area); };
	m_hasOnDestroySubscriptions.setCallback(onDestory);
	// Item.
	area.getItems().onDestroy_subscribe(m_item, m_hasOnDestroySubscriptions);
	// Recipient.
	area.getActors().onDestroy_subscribe(m_item, m_hasOnDestroySubscriptions);
}
