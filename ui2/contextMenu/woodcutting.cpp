#include "contextMenu.h"
#include "../window.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/plantSpecies.h"
bool plantIsValid(Window& window, const PlantIndex& plant)
{
	Plants& plants = window.m_area->getPlants();
	return PlantSpecies::getIsTree(plants.getSpecies(plant)) && plants.getPercentGrown(plant) >= Config::minimumPercentGrowthForWoodCutting;
}
bool designatedForCutting(Window& window, const PlantIndex& plant)
{
	Plants& plants = window.m_area->getPlants();
	return window.m_area->m_hasWoodCuttingDesignations.contains(window.m_faction, plants.getLocation(plant));
}
void contextMenu::controlls::woodcutting(Window& window)
{
	if(ImGui::BeginMenu("woodcutting"))
	{
		Space& space = window.m_area->getSpace();
		Plants& plants = window.m_area->getPlants();
		ControllsState& state = window.m_gameOverlay.m_controllsState;
		SmallSet<PlantIndex> selected;
		bool anyAreDesignatedForCutting = false;
		if(window.m_gameOverlay.m_selectMode == SelectMode::Plants || window.m_gameOverlay.m_selectMode == SelectMode::Space)
			space.plant_queryForEach(window.m_gameOverlay.m_selectedArea, [&](const PlantIndex& plant){
				if(plantIsValid(window, plant))
				{
					selected.insert(plant);
					if(designatedForCutting(window, plant))
						anyAreDesignatedForCutting = true;
				}
			});
		else
		{
			const PlantIndex& plant = space.plant_get(state.clickedOnPoint);
			if(plantIsValid(window, plant))
				selected.insert(plant);
		}
		if(selected.empty())
			return;
		auto& hasWoodCutting = window.m_area->m_hasWoodCuttingDesignations;
		if(ImGui::MenuItem("cut down"))
		{
			for(const PlantIndex& selectedPlant : selected)
				if(plantIsValid(window, selectedPlant) && !window.m_area->m_hasWoodCuttingDesignations.contains(window.m_faction, plants.getLocation(selectedPlant)))
					hasWoodCutting.designate(window.m_faction, plants.getLocation(selectedPlant));
			ImGui::CloseCurrentPopup();
		}
		if(anyAreDesignatedForCutting)
		{
			if(ImGui::MenuItem("cancel cut down"))
			{
				for(const PlantIndex& selectedPlant : selected)
					if(designatedForCutting(window, selectedPlant))
						hasWoodCutting.undesignate(window.m_faction, plants.getLocation(selectedPlant));
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::EndMenu();
	}
}