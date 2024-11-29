#include "equipItem.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../deserializationMemo.h"
#include "../area.h"
EquipItemObjective::EquipItemObjective(const ItemReference& item) : Objective(Config::equipPriority), m_item(item) { }
EquipItemObjective::EquipItemObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo)
{
	data["item"].get_to(m_item);
}
Json EquipItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	return data;
}
void EquipItemObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	ItemIndex item = m_item.getIndex(area.getItems().m_referenceData);
	if(!actors.isAdjacentToItem(actor, item))
		// detour, unresered, reserve.
		// TODO: detour.
		actors.move_setDestinationAdjacentToItem(actor, item, false, false, false);
	else
	{
		if(actors.equipment_canEquipCurrently(actor, item))
		{
			area.getItems().exit(item);
			actors.equipment_add(actor, item);
			actors.objective_complete(actor, *this);
		}
		else
			actors.objective_canNotCompleteObjective(actor, *this);
	}
}
void EquipItemObjective::cancel(Area& area, const ActorIndex& actor) { area.getActors().canReserve_clearAll(actor); }
void EquipItemObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
