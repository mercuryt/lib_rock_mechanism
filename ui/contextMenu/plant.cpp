#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/fluidType.h"
void ContextMenu::drawPlantControls(const BlockIndex& block)
{
	Area& area = *m_window.getArea();
	Blocks& blocks = area.getBlocks();
	Plants& plants = area.getPlants();
	if(blocks.plant_exists(block))
	{
		const PlantIndex& plant = blocks.plant_get(block);
		const PlantSpeciesId& species = plants.getSpecies(plant);
		std::wstring name = PlantSpecies::getName(species);
		auto label = tgui::Label::create();
		label->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, block, species, name, &plants, &blocks]{
			auto& submenu = makeSubmenu(0);
			auto infoButton = tgui::Button::create("info");
			submenu.add(infoButton);
			infoButton->onClick([this, block]{ m_window.getGameOverlay().drawInfoPopup(block);});
			if(m_window.m_editMode)
			{
				auto removePlant = tgui::Button::create(L"remove " + name);
				removePlant->onClick([this, block, species, &plants, &blocks]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
						if(blocks.plant_exists(selectedBlock) && plants.getSpecies(blocks.plant_get(selectedBlock)) == species)
							plants.remove(blocks.plant_get(selectedBlock));
					for(const PlantIndex& plant : m_window.getSelectedPlants())
						if(plants.getSpecies(plant) == species)
							plants.remove(plant);
					hide();
				});
				submenu.add(removePlant);
			}
		});
	}
	else if(m_window.m_editMode && blocks.plant_anythingCanGrowHereEver(block))
	{
		auto addPlant = tgui::Label::create("add plant");
		addPlant->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(addPlant);
		static Percent percentGrown = Percent::create(100);
		addPlant->onMouseEnter([this, &area, &blocks, block]{
			auto& submenu = makeSubmenu(0);
			auto speciesLabel = tgui::Label::create("species");
			speciesLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(speciesLabel);
			auto speciesUI = widgetUtil::makePlantSpeciesSelectUI(area, block);
			submenu.add(speciesUI);
			auto percentGrownUI = tgui::SpinControl::create();
			auto percentGrownLabel = tgui::Label::create("percent grown");
			submenu.add(percentGrownLabel);
			percentGrownLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(percentGrownUI);
			percentGrownUI->setMinimum(1);
			percentGrownUI->setMaximum(100);
			percentGrownUI->setValue(percentGrown.get());
			percentGrownUI->onValueChange([](const float value){ percentGrown = Percent::create(value); });
			auto confirm = tgui::Button::create("confirm");
			confirm->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(confirm);
			confirm->onClick([this, speciesUI, percentGrownUI, block, &blocks]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(!speciesUI->getSelectedItem().empty())
				{
					const PlantSpeciesId& species = PlantSpecies::byName(speciesUI->getSelectedItemId().toWideString());
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(const BlockIndex& selectedBlock : m_window.getSelectedBlocks().getView(blocks))
						if(!blocks.plant_exists(selectedBlock) && !blocks.solid_is(selectedBlock) && blocks.plant_canGrowHereEver(block, species))
							blocks.plant_create(selectedBlock, species, Percent::create(percentGrownUI->getValue()));
				}
				hide();
			});
		});
	}
}
