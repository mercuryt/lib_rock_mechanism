#include "contextMenu.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
void contextMenu::fluid(Window& window)
{
	assert(window.m_editMode);
	ContextMenuState& state = window.m_gameOverlay.m_contextMenuState;
	widgets::fluidType(&state.fluidType);
	ImGui::InputInt("volume", &state.fluidVolume);
	if(ImGui::Button("create fluid"))
	{
		std::lock_guard lock(window.m_simulation->m_uiReadMutex);
		window.m_area->getSpace().fluid_add(window.m_gameOverlay.m_selectedArea, {state.fluidVolume}, state.fluidType);
		state.current = ContextMenuId::Null;
	}
	if(ImGui::Button("create fluid source"))
	{
		std::lock_guard lock(window.m_simulation->m_uiReadMutex);
		window.m_area->m_fluidSources.create(
			window.getBlockUnderCursor(),
			state.fluidType,
			{state.fluidVolume}
		);
		state.current = ContextMenuId::Null;
	}
	if(window.m_area->m_fluidSources.contains(window.getBlockUnderCursor()))
	{
		if(ImGui::Button("remove fluid source"))
		{
			std::lock_guard lock(window.m_simulation->m_uiReadMutex);
			window.getArea()->m_fluidSources.destroy(window.m_gameOverlay.m_blockUnderCursor);
			state.current = ContextMenuId::Null;
		}
	}
	const Space& space = window.m_area->getSpace();
	if(space.fluid_any(window.m_gameOverlay.m_selectedArea))
	{
		if(ImGui::Button("remove fluid"))
		{
			std::lock_guard lock(window.m_simulation->m_uiReadMutex);
			for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
				for(const Point3D point : cuboid)
					window.m_area->getSpace().fluid_remove(point, {state.fluidVolume}, state.fluidType);
			state.current = ContextMenuId::Null;
		}
		if(ImGui::Button("select adjacent"))
		{
			window.m_gameOverlay.m_selectedArea = window.m_area->getSpace().fluid_getGroup(window.getBlockUnderCursor(), state.fluidType)->getPoints();
			state.current = ContextMenuId::Null;
		}
	}
}