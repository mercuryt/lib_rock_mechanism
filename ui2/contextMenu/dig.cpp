#include "contextMenu.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
void contextMenu::controlls::dig(Window& window)
{
	Space& space = window.m_area->getSpace();
	ControllsState& state = window.m_gameOverlay.m_controllsState;
	if(ImGui::BeginMenu("dig"))
	{
		if(ImGui::MenuItem("all"))
		{
			if(window.m_editMode)
			{
				space.solid_setNotAll(window.m_gameOverlay.m_selectedArea);
				for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
					for(const Point3D point : cuboid)
						space.pointFeature_removeAll(point);
			}
			else
			{
				for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
					for(const Point3D point : cuboid)
						window.m_area->m_hasDigDesignations.designate(window.m_faction, point, PointFeatureTypeId::Null);
			}
			ImGui::CloseCurrentPopup();
		}
		if(space.solid_isAny(state.clickedOnPoint) && space.solid_isAny(window.m_gameOverlay.m_selectedArea))
			if(ImGui::BeginMenu("hew feature"))
			{
				auto condition = [&](const PointFeatureTypeId featureType){
					return PointFeatureType::byId(featureType).canBeHewn;
				};
				widgets::featureType(&state.featureType, condition);
				if(ImGui::MenuItem("hew feature"))
				{
					const CuboidSet toHew = space.solid_queryCuboids(window.m_gameOverlay.m_selectedArea);
					if(window.m_editMode)
					{
						for(const Cuboid& cuboid : toHew)
							for(const Point3D point : cuboid)
								space.pointFeature_hew(point, state.featureType);
					}
					else
					{
						for(const Cuboid& cuboid : toHew)
							for(const Point3D point : cuboid)
								window.m_area->m_hasDigDesignations.designate(window.m_faction, point, state.featureType);
					}
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
		ImGui::EndMenu();
	}
}