#include "reference.h"
#include "area.h"
#include "actors/actors.h"
#include "items/items.h"
void ActorOrItemReference::load(const Json& data, Area& area)
{
	if(data[0])
	{
		ActorReference actor(data[1], area.getActors().m_referenceData);
		setActor(actor);
	}
	else
	{
		ItemReference item(data[1], area.getItems().m_referenceData);
		setItem(item);
	}
}