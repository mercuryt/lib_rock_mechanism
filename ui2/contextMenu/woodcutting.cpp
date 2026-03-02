#include "contextMenu.h"
#include "../window.h"
void contextMenu::woodcutting(Window& window)
{
	if(ImGui::Button("cut down"))
	{
		auto& hasWoodCutting = window.getArea()->m_hasWoodCuttingDesignations;
		if(m_window.getSelectedPlants().empty() && space.plant_exists(point) && plantIsValid(space.plant_get(point)))
			m_window.selectPlant(space.plant_get(point));
		for(const PlantIndex& selctedPlant : m_window.getSelectedPlants())
			if(plantIsValid(selctedPlant))
				hasWoodCutting.designate(faction, plants.getLocation(selctedPlant));
	}
	if(ImGui::Button("cancel"))
	{
		std::lock_guard lock(window.m_simulation->m_uiReadMutex);
		auto& hasWoodCutting = m_window.m_area->m_hasWoodCuttingDesignations;
		if(m_window.getSelectedPlants().empty() && space.plant_exists(point) && plantIsValid(space.plant_get(point)))
			m_window.selectPlant(space.plant_get(point));
		for(const PlantIndex& selctedPlant : window.getSelectedPlants())
			if(hasWoodCutting.contains(faction, plants.getLocation(selctedPlant)))
				hasWoodCutting.undesignate(faction, plants.getLocation(selctedPlant));
	}
}