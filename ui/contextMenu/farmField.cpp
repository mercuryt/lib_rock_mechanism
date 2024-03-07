#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/block.h"
void ContextMenu::drawFarmFieldControls(Block& block)
{
	// Farm
	if(m_window.getFaction() && block.m_isPartOfFarmField.contains(*m_window.getFaction()))
	{
		auto farmLabel = tgui::Label::create("farm");
		m_root.add(farmLabel);
		farmLabel->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		farmLabel->onMouseEnter([this, &block]{
			auto& submenu = makeSubmenu(0);
			auto shrinkButton = tgui::Button::create("shrink");
			shrinkButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(shrinkButton);
			shrinkButton->onClick([this]{
				for(Block* selectedBlock : m_window.getSelectedBlocks())
					selectedBlock->m_isPartOfFarmField.remove(*m_window.getFaction());
				hide();
			});
			auto setFarmSpeciesButton = tgui::Button::create("set species");
			setFarmSpeciesButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(setFarmSpeciesButton);
			setFarmSpeciesButton->onClick([this, &block]{
				auto& submenu = makeSubmenu(1);
				auto plantSpeciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
				submenu.add(plantSpeciesUI);
				plantSpeciesUI->onItemSelect([this, &block](const tgui::String& string)
				{
					const PlantSpecies& species = PlantSpecies::byName(string.toStdString());
					FarmField& field = *block.m_isPartOfFarmField.get(*m_window.getFaction());
					if(&species != field.plantSpecies)
					{
						auto& hasFarmFields = block.m_area->m_hasFarmFields.at(*m_window.getFaction());
						if(field.plantSpecies)
							hasFarmFields.clearSpecies(field);
						hasFarmFields.setSpecies(field, species);
						hide();
					}
					
				});
			});
		});
	}
	else if(m_window.getFaction() && block.getBlockBelow() && block.getBlockBelow()->m_hasPlant.anythingCanGrowHereEver())
	{
		auto createLabel = tgui::Label::create("create farm field");	
		createLabel->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(createLabel);
		createLabel->onMouseEnter([this, &block]{
			auto& submenu = makeSubmenu(0);
			auto plantSpeciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
			submenu.add(plantSpeciesUI);
			plantSpeciesUI->onItemSelect([this, &block](const tgui::String name){ 
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(block);
				if(!m_window.getArea()->m_hasFarmFields.contains(*m_window.getFaction()))
					m_window.getArea()->m_hasFarmFields.registerFaction(*m_window.getFaction());
				auto& fieldsForFaction = block.m_area->m_hasFarmFields.at(*m_window.getFaction());
				FarmField& field = fieldsForFaction.create(m_window.getSelectedBlocks());
				fieldsForFaction.setSpecies(field, PlantSpecies::byName(name.toStdString()));
				hide();
			});
		});
	}
}
