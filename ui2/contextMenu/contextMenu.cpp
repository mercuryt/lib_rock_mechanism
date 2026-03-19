#include "contextMenu.h"
#include "../window.h"
#include "../../engine/space/space.h"
#include "../../engine/area/area.h"
#include "../../engine/definitions/plantSpecies.h"
#include "../../engine/definitions/animalSpecies.h"
void contextMenu::draw(Window& window)
{
	const Space& space = window.m_area->getSpace();
	if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))
	{
		ImGui::OpenPopup("contextMenu");
		window.m_gameOverlay.m_controllsState.clickedOnPoint = window.m_gameOverlay.m_blockUnderCursor;
	}
	if(ImGui::BeginPopup("contextMenu"))
	{
		const Point3D point = window.m_gameOverlay.m_controllsState.clickedOnPoint;
		if(ImGui::MenuItem("location info"))
		{
			window.m_gameOverlay.m_detailPoint = point;
			window.m_gameOverlay.m_infoPopUp = InfoPopUpId::Point;
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