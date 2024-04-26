#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
void ContextMenu::drawPlantControls(Block& block)
{
	if(block.m_hasPlant.exists())
	{
		auto label = tgui::Label::create(block.m_hasPlant.get().m_plantSpecies.name);
		label->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, &block]{
			auto& submenu = makeSubmenu(0);
			auto infoButton = tgui::Button::create("info");
			submenu.add(infoButton);
			infoButton->onClick([this, &block]{ m_window.getGameOverlay().drawInfoPopup(block);});
			if(m_window.m_editMode)
			{
				auto removePlant = tgui::Button::create("remove " + block.m_hasPlant.get().m_plantSpecies.name);
				removePlant->onClick([this, &block]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					const PlantSpecies& species = block.m_hasPlant.get().m_plantSpecies;
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block* selectedBlock : m_window.getSelectedBlocks())
						if(selectedBlock->m_hasPlant.exists() && selectedBlock->m_hasPlant.get().m_plantSpecies == species)
							selectedBlock->m_hasPlant.get().remove();
					for(Plant* plant : m_window.getSelectedPlants())
						if(plant->m_plantSpecies == species)
							plant->remove();
					hide();
				});
				submenu.add(removePlant);
			}
		});
	} 
	else if(m_window.m_editMode && block.m_hasPlant.anythingCanGrowHereEver())
	{
		auto addPlant = tgui::Label::create("add plant");
		addPlant->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(addPlant);
		static Percent percentGrown = 100;
		addPlant->onMouseEnter([this, &block]{
			auto& submenu = makeSubmenu(0);
			auto speciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
			submenu.add(speciesUI);
			auto percentGrownUI = tgui::SpinControl::create();
			auto percentGrownLabel = tgui::Label::create("percent grown");
			submenu.add(percentGrownLabel);
			percentGrownLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(percentGrownUI);
			percentGrownUI->setMinimum(1);
			percentGrownUI->setMaximum(100);
			percentGrownUI->setValue(percentGrown);
			percentGrownUI->onValueChange([](const float value){ percentGrown = value; });
			auto confirm = tgui::Button::create("confirm");
			confirm->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(confirm);
			confirm->onClick([this, speciesUI, percentGrownUI, &block]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(!speciesUI->getSelectedItem().empty())
				{
					const PlantSpecies& species = PlantSpecies::byName(speciesUI->getSelectedItemId().toStdString());
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block* selectedBlock : m_window.getSelectedBlocks())
						if(!selectedBlock->m_hasPlant.exists() && !selectedBlock->isSolid() && selectedBlock->m_hasPlant.canGrowHereEver(species))
							selectedBlock->m_hasPlant.createPlant(species, percentGrownUI->getValue());
				}
				hide();
			});
		});
	}
}
