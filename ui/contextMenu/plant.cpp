#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/plantSpecies.h"
#include "../../engine/fluidType.h"
void ContextMenu::drawPlantControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	Plants& plants = area.getPlants();
	if(space.plant_exists(point))
	{
		const PlantIndex& plant = space.plant_get(point);
		const PlantSpeciesId& species = plants.getSpecies(plant);
		std::string name = PlantSpecies::getName(species);
		auto label = tgui::Label::create();
		label->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, point, species, name, &plants, &space]{
			auto& submenu = makeSubmenu(0);
			auto infoButton = tgui::Button::create("info");
			submenu.add(infoButton);
			infoButton->onClick([this, point]{ m_window.getGameOverlay().drawInfoPopup(point);});
			if(m_window.m_editMode)
			{
				auto removePlant = tgui::Button::create("remove " + name);
				removePlant->onClick([this, point, species, &plants, &space]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(point);
					for(const Cuboid& cuboid : m_window.getSelectedBlocks())
						for(const Point3D& selectedBlock : cuboid)
							if(space.plant_exists(selectedBlock) && plants.getSpecies(space.plant_get(selectedBlock)) == species)
								plants.remove(space.plant_get(selectedBlock));
					for(const PlantIndex& selectedPlant : m_window.getSelectedPlants())
						if(plants.getSpecies(selectedPlant) == species)
							plants.remove(selectedPlant);
					hide();
				});
				submenu.add(removePlant);
			}
		});
	}
	else if(m_window.m_editMode && space.plant_anythingCanGrowHereEver(point))
	{
		auto addPlant = tgui::Label::create("add plant");
		addPlant->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(addPlant);
		static Percent percentGrown = Percent::create(100);
		addPlant->onMouseEnter([this, &area, &space, point]{
			auto& submenu = makeSubmenu(0);
			auto speciesLabel = tgui::Label::create("species");
			speciesLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(speciesLabel);
			auto speciesUI = widgetUtil::makePlantSpeciesSelectUI(area, point);
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
			confirm->onClick([this, speciesUI, percentGrownUI, point, &space]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(!speciesUI->getSelectedItem().empty())
				{
					const PlantSpeciesId& species = PlantSpecies::byName(speciesUI->getSelectedItemId().toStdString());
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(point);
					for(const Cuboid& cuboid : m_window.getSelectedBlocks())
						for(const Point3D& selectedBlock : cuboid)
							if(!space.plant_exists(selectedBlock) && !space.solid_isAny(selectedBlock) && space.plant_canGrowHereEver(point, species))
								space.plant_create(selectedBlock, species, Percent::create(percentGrownUI->getValue()));
				}
				hide();
			});
		});
	}
}
