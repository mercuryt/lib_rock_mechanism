#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
bool blockIsValid(const Block& block)
{
	return block.m_hasPlant.exists() && block.m_hasPlant.get().m_plantSpecies.isTree && block.m_hasPlant.get().getGrowthPercent() >= Config::minimumPercentGrowthForWoodCutting;
}
void ContextMenu::drawWoodCuttingControls(Block& block)
{
	if(m_window.getFaction() && blockIsValid(block))
	{
		auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
		const Faction& faction = *m_window.getFaction();
		if(hasWoodCutting.contains(faction, block))
		{
			auto button = tgui::Button::create("cancel fell trees");
			m_root.add(button);
			button->onClick([this]{
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				const Faction& faction = *m_window.getFaction();
				for(Block* selctedBlock : m_window.getSelectedBlocks())
					if(hasWoodCutting.contains(faction, *selctedBlock))
						hasWoodCutting.undesignate(faction, *selctedBlock);
			});
		}
		else 
		{
			auto button = tgui::Button::create("fell trees");
			m_root.add(button);
			button->onClick([this]{
				auto& hasWoodCutting = m_window.getArea()->m_hasWoodCuttingDesignations;
				const Faction& faction = *m_window.getFaction();
				for(Block* selctedBlock : m_window.getSelectedBlocks())
					if(blockIsValid(*selctedBlock))
						hasWoodCutting.designate(faction, *selctedBlock);
			});
		}
	}
}

