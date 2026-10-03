#include "screens.h"
#include "../window.h"
#include "../../engine/expedition/expedition.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
void screens::expeditionDetails(Window& window, ExpeditionId id)
{
	window.m_paused = true;
	const Expedition& expedition = window.m_simulation->m_hasExpeditions.byId(id);
	const Area& area = window.m_simulation->m_hasAreas->getById(expedition.area);
	const Actors& actors = area.getActors();
	const Items& items = area.getItems();
	begin(window, expedition.name);
	if(expedition.faction.exists())
		// TODO: link to faction details.
		ImGuiText(window.m_simulation->m_hasFactions.getById(expedition.faction).name);
	for(ActorIndex actor : actors.getAll())
		if(ImGui::Button(actors.getName(actor).c_str()))
			actorDetails(window, actor);
	for(ItemIndex item : items.getAll())
		ImGuiText(items.description(item));
	if(window.m_editMode || window.m_faction == expedition.faction)
		if(ImGui::Button("Select"))
		{
			window.m_mapOverlay.m_selectedExpedition = id;
			window.showGame();
		}
		if(ImGui::Button("View"))
		{
			window.m_area = &window.m_simulation->m_world->getOrCreateArea(*window.m_simulation, expedition.location);
			window.m_mapOpen = false;
			window.showGame();
		}
	if(ImGui::Button("Close"))
		window.showGame();
	end();
}