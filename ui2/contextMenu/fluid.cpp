#include "contextMenu.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
void contextMenu::controlls::fluid(Window& window)
{
	assert(window.m_editMode);
	if(ImGui::BeginMenu("fluid"))
	{
		ControllsState& state = window.m_gameOverlay.m_controllsState;
		widgets::fluidType(&state.fluidType);
		ImGui::InputInt("volume", &state.fluidVolume);
		if(ImGui::MenuItem("create fluid"))
		{
			window.m_area->getSpace().fluid_add(window.m_gameOverlay.m_selectedArea, {state.fluidVolume}, state.fluidType);
			ImGui::CloseCurrentPopup();
		}
		if(ImGui::MenuItem("create fluid source"))
		{
			window.m_area->m_fluidSources.create(
				state.clickedOnPoint,
				state.fluidType,
				{state.fluidVolume}
			);
			ImGui::CloseCurrentPopup();
		}
		if(window.m_area->m_fluidSources.contains(state.clickedOnPoint))
		{
			if(ImGui::MenuItem("remove fluid source"))
			{
				auto& fluidSources = window.m_area->m_fluidSources;
				for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
					for(const Point3D point : cuboid)
						if(fluidSources.contains(point))
							fluidSources.destroy(point);
				ImGui::CloseCurrentPopup();
			}
		}
		const Space& space = window.m_area->getSpace();
		if(space.fluid_any(window.m_gameOverlay.m_selectedArea))
		{
			if(ImGui::MenuItem("remove fluid"))
			{
				for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
					for(const Point3D point : cuboid)
						window.m_area->getSpace().fluid_removeSyncronus(point, {state.fluidVolume}, state.fluidType);
				ImGui::CloseCurrentPopup();
			}
			if(ImGui::MenuItem("select adjacent"))
			{
				const CuboidSet fluidGroupArea = window.m_area->getSpace().fluid_getGroup(state.clickedOnPoint, state.fluidType)->getPoints();
				if(window.m_controllKey)
					window.m_gameOverlay.m_selectedArea.maybeRemoveAll(fluidGroupArea);
				else
				{
					if(!window.m_shiftKey)
						window.m_gameOverlay.m_selectedArea.clear();
					window.m_gameOverlay.m_selectedArea.addAll(fluidGroupArea);
				}
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndMenu();
	}
}