#include "giveItem.h"
#include "../actors/actors.h"
#include "../deserializationMemo.h"
#include "area/area.h"
#include "items/items.h"
#include "onDestroy.h"
GiveItemObjective::GiveItemObjective(Area& area, const ItemIndex& item, const ActorIndex& recipient) :
	Objective(Config::equipPriority)
{

	m_item.setIndex(item, area.getItems().m_referenceData);
	m_recipient.setIndex(recipient, area.getActors().m_referenceData);
	createOnDestroyCallbacks(area, recipient);
}
GiveItemObjective::GiveItemObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{
	m_item.load(data["item"], area.getItems().m_referenceData);
	m_recipient.load(data["recipient"], area.getActors().m_referenceData);
	createOnDestroyCallbacks(area, m_recipient.getIndex(area.getActors().m_referenceData));
}
Json GiveItemObjective::toJson() const
{
	return {{"item", m_item}, {"recipient", m_recipient}};
}
void GiveItemObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	ActorIndex recipient = m_recipient.getIndex(actors.m_referenceData);
	ItemIndex item = m_item.getIndex(items.m_referenceData);
	if(!actors.isAdjacentToActor(actor, recipient))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToActor(actor, recipient);
	else
	{
		if(actors.equipment_canEquipCurrently(recipient, item))
		{
			actors.equipment_remove(actor, item);
			actors.equipment_add(recipient, item);
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
	Items& items = area.getItems();
	ActorReference actorRef = area.getActors().m_referenceData.getReference(actor);
	m_hasOnDestroySubscriptions.setCallback(std::make_unique<CancelObjectiveOnDestroyCallBack>(actorRef, *this, area));
	// Item.
	items.onDestroy_subscribe(m_item.getIndex(items.m_referenceData), m_hasOnDestroySubscriptions);
	// Recipient.
	area.getActors().onDestroy_subscribe(actor, m_hasOnDestroySubscriptions);
}
