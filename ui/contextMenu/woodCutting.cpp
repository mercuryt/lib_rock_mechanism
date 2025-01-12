#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
bool ContextMenu::plantIsValid(const PlantIndex& plant) const
{
	Plants& plants = m_window.getArea()->getPlants();
	return PlantSpecies::getIsTree(plants.getSpecies(plant)) && plants.getPercentGrown(plant) >= Config::minimumPercentGrowthForWoodCutting;
}
void ContextMenu::drawWoodCuttingControls(const BlockIndex& block)
{
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	const FactionId& faction = m_window.getFaction();
	if(faction.exists() && blocks.plant_exists(block) && plantIsValid(blocks.plant_get(block)))
	{
		auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
		if(faction.exists() && hasWoodCutting.contains(faction, block))
		{
			auto button = tgui::Button::create("cancel fell trees");
			m_root.add(button);
			button->onClick([this, &blocks, &plants, block, faction]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				if(m_window.getSelectedPlants().empty() && blocks.plant_exists(block) && plantIsValid(blocks.plant_get(block)))
					m_window.selectPlant(blocks.plant_get(block));
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
			button->onClick([this, &blocks, &plants, block, faction]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				if(m_window.getSelectedPlants().empty() && blocks.plant_exists(block) && plantIsValid(blocks.plant_get(block)))
					m_window.selectPlant(blocks.plant_get(block));
				for(const PlantIndex& selctedPlant : m_window.getSelectedPlants())
					if(plantIsValid(selctedPlant))
						hasWoodCutting.designate(faction, plants.getLocation(selctedPlant));
				hide();
			});
		}
	}
}
