#include "contextMenu.h"
#include "../window.h"
#include "../../engine/space/space.h"
#include "../../engine/area/area.h"
#include "../../engine/definitions/plantSpecies.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/simulation/hasAreas.h"
void contextMenu::drawArea(Window& window)
{
	const Space& space = window.m_area->getSpace();
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("contextMenu");
		window.m_areaOverlay.m_controllsState.clickedOnPoint = window.m_blockUnderCursor;
	}
	if(ImGui::BeginPopup("contextMenu"))
	{
		const Point3D point = window.m_areaOverlay.m_controllsState.clickedOnPoint;
		if(ImGui::MenuItem("location info"))
		{
			window.m_areaOverlay.m_detailPoint = point;
			window.m_areaOverlay.m_infoPopUp = InfoPopUpId::Point;
			ImGui::CloseCurrentPopup();
		}
		if(space.solid_isAny(point) || !space.pointFeature_empty(point))
			controlls::dig(window);
		else
		{
			controlls::construct(window);
			if(window.m_editMode)
				controlls::fluid(window);
			controlls::plants(window);
			controlls::items(window);
			controlls::actors(window);
			//controlls::woodcutting(window);
		}
		if(window.m_editMode)
		{
			if(ImGui::MenuItem("edit drama"))
				window.m_panel = PanelId::EditDrama;
			if(ImGui::MenuItem("edit factions"))
				window.m_panel = PanelId::SelectFactionToEdit;
		}
		ImGui::EndPopup();
	}
}
void contextMenu::drawMap(Window& window)
{
	Simulation& simulation = *window.m_simulation;
	World& world = *simulation.m_world;
	bool canIssueMoveCommand = false;
	bool anyExpeditionsAtLocation = false;
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		if(window.m_mapOverlay.m_selectedExpedition.exists())
		{
			Expedition& expedition = simulation.m_hasExpeditions.byId(window.m_mapOverlay.m_selectedExpedition);
			if(
				window.m_faction == expedition.faction &&
				expedition.location != window.m_blockUnderCursor &&
				expedition.path.back() != window.m_blockUnderCursor
			)
				canIssueMoveCommand = true;
		}
		anyExpeditionsAtLocation = world.m_expeditions.queryAny(window.m_blockUnderCursor);
		if(canIssueMoveCommand || anyExpeditionsAtLocation)
			ImGui::OpenPopup("mapContextMenu");
		else
			window.setArea(simulation.m_hasAreas->getById(world.m_areas.queryGetOne(window.m_blockUnderCursor)));
	}
	if(ImGui::BeginPopup("mapContextMenu"))
	{
		if(canIssueMoveCommand && ImGui::Button("Set Destination"))
		{
			Expedition& expedition = simulation.m_hasExpeditions.byId(window.m_mapOverlay.m_selectedExpedition);
			expedition.setDestination(window.m_blockUnderCursor);
		}
		if(anyExpeditionsAtLocation)
		{
			for(ExpeditionId id : world.m_expeditions.queryGetAll(window.m_blockUnderCursor))
			{
				Expedition& expedition = simulation.m_hasExpeditions.byId(id);
				if(ImGui::Button(expedition.name.c_str()))
				{
					// TODO: currently m_selectedExpediton reperesents both the expedition being given movement commands and the expedition show on the details screen, should these be seperated?
					window.m_mapOverlay.m_selectedExpedition = id;
					window.m_panel = PanelId::ExpeditionDetails;
				}
			}
		}
		ImGui::EndPopup();
	}
}