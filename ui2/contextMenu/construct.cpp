#include "contextMenu.h"
#include "../window.h"
void contextMenu::controlls::construct(Window& window)
{
	if(ImGui::Button("construct"))
	{
		Space& space = window.m_area->getSpace();
		ContextMenuState& state = window.m_gameOverly.m_contextMenuState;
		CuboidSet toConstruct = window.m_gameOverlay.m_selectedArea;
		space.solid_forEachCuboid(window.m_gameOverlay.m_selectedArea, [&](const Cuboid cuboid){ toConstruct.remove(cuboid); });
		space.feature_forEachCuboid(window.m_gameOverlay.m_selectedArea, [&](const Cuboid cuboid){ toConstruct.remove(cuboid); });
		// Currently unrevealed cannot be selected so this is redundant. Eventually it will be neccesary as that restriction is removed to allow provisional dig designations.
		space.unrevealed_forEachCuboid(window.m_gameOverlay.m_selectedArea, [&](const Cuboid cuboid){ toConstruct.remove(cuboid); });
		if(window.editMode)
		{
			if(state.pointFeatureType == PointFeatureTypeId::Null)
				window.m_area->getSpace().solid_set(toConstruct, state.materialType, state.constructed);
			else
			{
				if(!state.constructed)
				{
					if(!space.solid_isAny(toConstruct))
						space.solid_set(toConstruct, state.materialType, false);
					space.pointFeature_hew(toConstruct, state.featureType));
				}
				else
				{
					for(const Cuboid& cuboid : toConstruct)
						for(const Point3D point : cuboid)
							space.pointFeature_construct(pont, state.featureType, state.materialType);
				}
			}

		}
		else
			for(const Cuboid& cuboid : toConstruct)
				for(const Point3D point : cuboid)
					m_window.getArea()->m_hasConstructionDesignations.designate(m_window.m_faction, point, state.pointFeatureType, state.materialType);
		state.current = ContextMenuId::Null;
	}
	ImGui::SameLine();
	if(ImGui::Button("..."))
		state.current = ContextMenuId::Construct;
}
void contextMenu::submenus::construct(Window& window)
{
	ContextMenuState& state = window.m_gameOverly.m_contextMenuState;
	widgets::materialType(&state.materialType);
	widgets::pointFeatureType(&state.featureType);
	if(window.m_editMode)
		ImGui::Checkbox("constructed", &state.constructed);
	if(ImGui::Button("back"))
		state.current = ContextMenuId::Root;
}