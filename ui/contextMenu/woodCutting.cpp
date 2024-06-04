#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
bool plantIsValid(const Plant& plant)
{
	return plant.m_plantSpecies.isTree && plant.getGrowthPercent() >= Config::minimumPercentGrowthForWoodCutting;
}
void ContextMenu::drawWoodCuttingControls(BlockIndex& block)
{
	if(m_window.getFaction() && block.m_hasPlant.exists() && plantIsValid(block.m_hasPlant.get()))
	{
		auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
		Faction& faction = *m_window.getFaction();
		if(hasWoodCutting.contains(faction, block))
		{
			auto button = tgui::Button::create("cancel fell trees");
			m_root.add(button);
			button->onClick([this, &block]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				Faction& faction = *m_window.getFaction();
				if(m_window.getSelectedPlants().empty() && block.m_hasPlant.exists() && plantIsValid(block.m_hasPlant.get()))
					m_window.selectPlant(block.m_hasPlant.get());
				for(Plant* selctedPlant : m_window.getSelectedPlants())
					if(hasWoodCutting.contains(faction, *selctedPlant->m_location))
						hasWoodCutting.undesignate(faction, *selctedPlant->m_location);
				hide();
			});
		}
		else 
		{
			auto button = tgui::Button::create("fell trees");
			m_root.add(button);
			button->onClick([this, &block]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				Faction& faction = *m_window.getFaction();
				if(m_window.getSelectedPlants().empty() && block.m_hasPlant.exists() && plantIsValid(block.m_hasPlant.get()))
					m_window.selectPlant(block.m_hasPlant.get());
				for(Plant* selctedPlant : m_window.getSelectedPlants())
					if(plantIsValid(*selctedPlant))
						hasWoodCutting.designate(faction, *selctedPlant->m_location);
				hide();
			});
		}
	}
}

