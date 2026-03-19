#include "contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../widgets/widgets.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/plantSpecies.h"
#include "../../engine/fluidType.h"
void contextMenu::controlls::plants(Window& window)
{
	if(ImGui::BeginMenu("plants"))
	{
		Area& area = *window.getArea();
		Space& space =  area.getSpace();
		Plants& plants = area.getPlants();
		ControllsState& state = window.m_gameOverlay.m_controllsState;
		if(space.plant_exists(state.clickedOnPoint))
		{
			const PlantIndex& plant = space.plant_get(state.clickedOnPoint);
			const PlantSpeciesId& species = plants.getSpecies(plant);
			if(ImGui::BeginMenu(PlantSpecies::getName(species).c_str()))
			{
				if(ImGui::MenuItem("info"))
				{
					window.m_gameOverlay.m_detailPoint = state.clickedOnPoint;
					window.m_gameOverlay.m_infoPopUp = InfoPopUpId::Plant;
					ImGui::CloseCurrentPopup();
				}
				if(window.m_editMode)
				{
					if(ImGui::MenuItem("remove"))
					{
						// Remove all selected with the same species as the one clicked on.
						if(window.m_gameOverlay.m_selectedArea.empty())
							window.m_gameOverlay.m_selectedArea.add(state.clickedOnPoint);
						for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
							for(const Point3D& selectedBlock : cuboid)
								if(space.plant_exists(selectedBlock) && plants.getSpecies(space.plant_get(selectedBlock)) == species)
									plants.remove(space.plant_get(selectedBlock));
						ImGui::CloseCurrentPopup();
					}

				}
				ImGui::EndMenu();
			}
		}
		else if(window.m_editMode && space.plant_anythingCanGrowHereEver(state.clickedOnPoint))
		{
			if(ImGui::BeginMenu("create"))
			{
				widgets::plantSpecies(&state.plantSpecies);
				ImGui::InputInt("percent grown", &state.percentGrown.getReference());
				if(ImGui::MenuItem("confirm"))
				{
					if(window.m_gameOverlay.m_selectedArea.empty())
							window.m_gameOverlay.m_selectedArea.add(state.clickedOnPoint);
					for(const Cuboid& cuboid : window.m_gameOverlay.m_selectedArea)
						for(const Point3D& selectedBlock : cuboid)
							if(!space.plant_exists(selectedBlock) && !space.solid_isAny(selectedBlock) && space.plant_canGrowHereEver(state.clickedOnPoint, state.plantSpecies))
								space.plant_create(selectedBlock, state.plantSpecies, state.percentGrown);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenu();
	}
}
