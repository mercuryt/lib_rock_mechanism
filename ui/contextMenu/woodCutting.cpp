#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
#include "../../engine/plants.h"
bool ContextMenu::plantIsValid(const PlantIndex& plant) const
{
	Plants& plants = m_window.getArea()->getPlants();
	return PlantSpecies::getIsTree(plants.getSpecies(plant)) && plants.getPercentGrown(plant) >= Config::minimumPercentGrowthForWoodCutting;
}
void ContextMenu::drawWoodCuttingControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	Plants& plants = area.getPlants();
	const FactionId& faction = m_window.getFaction();
	if(faction.exists() && space.plant_exists(point) && plantIsValid(space.plant_get(point)))
	{
		auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
		if(faction.exists() && hasWoodCutting.contains(faction, point))
		{
			auto button = tgui::Button::create("cancel fell trees");
			m_root.add(button);
			button->onClick([this, &space, &plants, point, faction]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				if(m_window.getSelectedPlants().empty() && space.plant_exists(point) && plantIsValid(space.plant_get(point)))
					m_window.selectPlant(space.plant_get(point));
				for(const PlantIndex& selctedPlant : m_window.getSelectedPlants())
					if(hasWoodCutting.contains(faction, plants.getLocation(selctedPlant)))
						hasWoodCutting.undesignate(faction, plants.getLocation(selctedPlant));
				hide();
			});
		}
		else
		{
			auto button = tgui::Button::create("fell trees");
			m_root.add(button);
			button->onClick([this, &space, &plants, point, faction]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				if(m_window.getSelectedPlants().empty() && space.plant_exists(point) && plantIsValid(space.plant_get(point)))
					m_window.selectPlant(space.plant_get(point));
				for(const PlantIndex& selctedPlant : m_window.getSelectedPlants())
					if(plantIsValid(selctedPlant))
						hasWoodCutting.designate(faction, plants.getLocation(selctedPlant));
				hide();
			});
		}
	}
}
