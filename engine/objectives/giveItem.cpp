#include "giveItem.h"
#include "../actors/actors.h"
#include "../deserializationMemo.h"
#include "area.h"
#include "items/items.h"
#include "onDestroy.h"
GiveItemObjective::GiveItemObjective(Area& area, const ItemIndex& item, const ActorIndex& recipient) :
	Objective(Config::equipPriority)
{ 
	m_item.setTarget(area.getItems().getReferenceTarget(item));
	m_recipient.setTarget(area.getActors().getReferenceTarget(recipient));
	createOnDestroyCallbacks(area, recipient);
}
GiveItemObjective::GiveItemObjective(const Json& data, Area& area) :
	Objective(data)
{
	m_item.load(data["item"], area);
	m_recipient.load(data["recipent"], area);
	createOnDestroyCallbacks(area, m_recipient.getIndex());
}
Json GiveItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	data["recipient"] = m_recipient;
	return data;
}
void GiveItemObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	if(!actors.isAdjacentToActor(actor, m_recipient.getIndex()))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToActor(actor, m_recipient.getIndex());
	else
	{
		if(actors.equipment_canEquipCurrently(m_recipient.getIndex(), m_item.getIndex()))
		{
			actors.equipment_remove(actor, m_item.getIndex());
			actors.equipment_add(m_recipient.getIndex(), m_item.getIndex());
			actors.objective_complete(actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(actor, *this);
	}
}
void GiveItemObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().canReserve_clearAll(actor); }
void GiveItemObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void GiveItemObjective::createOnDestroyCallbacks(Area& area, const ActorIndex& actor) 
{ 
	m_hasOnDestroySubscriptions.setCallback(std::make_unique<CancelObjectiveOnDestroyCallBack>(actor.toReference(area), *this, area));
	// Item.
	area.getItems().onDestroy_subscribe(m_item.getIndex(), m_hasOnDestroySubscriptions);
	// Recipient.
	area.getActors().onDestroy_subscribe(actor, m_hasOnDestroySubscriptions);
}
