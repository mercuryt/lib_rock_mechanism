#include "contextMenu.h"
#include "../window.h"
#include "../widgets/widgets.h"
void contextMenu::controlls::dig(Window& window)
{
	Space& space = window.m_area->getSpace();
	if(ImGui::Button("dig all"))
	{
		if(window.editMode)
		{
			space.solid_unset(window.m_gameOverlay.m_selectedArea);
			space.feature_removeAll(window.m_gameOverlay.m_selectedArea);
		}
		else
		{
			for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
				for(const Point3D point : cuboid)
					m_window.getArea()->m_hasDigDesignations.designate(m_window.m_faction, point, PointFeatureTypeId::Null);
		}
		state.current = ContextMenuId::Null;
	}
	if(space.solid_isAny(state.clickedOnPoint) && space.solid_isAny(window.m_gameOverlay.m_selectedArea))
		if(ImGui::Button("hew feature"))
			state.current = ContextMenuId::Dig;
}
void contextMenu::submenus::dig(Window& window)
{
	ContextMenuState& state = window.m_gameOverly.m_contextMenuState;
	Space& space = window.m_area->getSpace();
	auto condition = [&](const PointFeatureTypeId featureType){
		return PointFeature::byId(featureType).canBeHewn;
	};
	widgets::featureType(&state.featureType, condition);
	if(ImGui::Button("hew feature"))
	{
		const CuboidSet toHew = space.solid_queryCuboids(window.m_gameOverlay.m_selectedArea);
		space.solid_removeNot(selectedCopy);
		if(window.editMode)
			space.pointFeature_hew(toHew, *state.featureType);
		else
		{
			for(const Cuboid& cuboid : toHew)
				for(const Point3D point : cuboid)
					m_window.getArea()->m_hasDigDesignations.designate(m_window.m_faction, point, *state.featureType);
		}
		state.current = ContextMenuId::Null;
	}
}