#include "contextMenu.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
#include "../../engine/pointFeature.h"
void contextMenu::controlls::construct(Window& window)
{
	ControllsState& state = window.m_gameOverlay.m_controllsState;
	if(ImGui::BeginMenu("construct"))
	{
		widgets::materialType(&state.materialType);
		if(window.m_editMode)
			ImGui::Checkbox("constructed", &state.constructed);
		if(state.materialType.exists())
		{
			std::string solidAction = "solid " + MaterialType::getName(state.materialType);
			if(ImGui::MenuItem(solidAction.c_str()))
			{
				contextMenu::helpers::construct(window, false);
				ImGui::CloseCurrentPopup();
			}
		}
		if(ImGui::BeginMenu("feature"))
		{
			widgets::featureType(&state.featureType);
			std::string featureAction = MaterialType::getName(state.materialType) + " " + PointFeatureType::byId(state.featureType).name;
			if(ImGui::MenuItem(featureAction.c_str()))
			{
				contextMenu::helpers::construct(window, true);
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndMenu();
		}
		ImGui::EndMenu();
	}
}
void contextMenu::helpers::construct(Window& window, bool feature)
{
	CuboidSet toConstruct = window.m_gameOverlay.m_selectedArea;
	Space& space = window.m_area->getSpace();
	ControllsState& state = window.m_gameOverlay.m_controllsState;
	space.solid_queryForEachCuboid(window.m_gameOverlay.m_selectedArea, [&](const Cuboid cuboid){ toConstruct.remove(cuboid); });
	space.pointFeature_queryForEachCuboid(window.m_gameOverlay.m_selectedArea, [&](const Cuboid cuboid){ toConstruct.remove(cuboid); });
	if(window.m_editMode)
	{
		if(!feature)
			window.m_area->getSpace().solid_setAll(toConstruct, state.materialType, state.constructed);
		else
		{
			if(!state.constructed)
			{
				if(!space.solid_isAny(toConstruct))
					space.solid_setAll(toConstruct, state.materialType, false);
				for(const Cuboid& cuboid : toConstruct)
					for(const Point3D point : cuboid)
						space.pointFeature_hew(point, state.featureType);
			}
			else
			{
				for(const Cuboid& cuboid : toConstruct)
					for(const Point3D point : cuboid)
						space.pointFeature_construct(point, state.featureType, state.materialType);
			}
		}
	}
	else
		for(const Cuboid& cuboid : toConstruct)
			for(const Point3D point : cuboid)
				window.getArea()->m_hasConstructionDesignations.designate(window.m_faction, point, state.featureType, state.materialType);
}