#include "giveItem.h"
#include "../actor.h"
#include "../block.h"
GiveItemObjective::GiveItemObjective(Actor& actor, Item& item, Actor& recipient) : Objective(actor, Config::equipPriority), m_item(item), m_recipient(recipient) 
{ 
	createOnDestroyCallbacks();
}

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
void GiveItemObjective::execute()
{
	if(!m_actor.isAdjacentTo(m_recipient))
		// detour, unresered, reserve.
		// TODO: detour.
		m_actor.m_canMove.setDestinationAdjacentTo(m_recipient, false, false, false);
	else
	{
		if(m_recipient.m_equipmentSet.canEquipCurrently(m_item))
		{
			m_actor.m_equipmentSet.removeEquipment(m_item);
			m_recipient.m_equipmentSet.addEquipment(m_item);
			m_actor.m_hasObjectives.objectiveComplete(*this);
		}
		else
			m_actor.m_hasObjectives.cannotFulfillObjective(*this);
	}
}
void GiveItemObjective::cancel() { m_actor.m_canReserve.deleteAllWithoutCallback(); }
void GiveItemObjective::reset() { cancel(); }
void GiveItemObjective::createOnDestroyCallbacks() 
{ 
	std::function<void()> onDestory = [this]{ cancel(); };
	m_hasOnDestroySubscriptions.setCallback(onDestory);
	// Item.
	m_item.m_onDestroy.subscribe(m_hasOnDestroySubscriptions);
	// Recipient.
	m_actor.m_onDestroy.subscribe(m_hasOnDestroySubscriptions);
}
