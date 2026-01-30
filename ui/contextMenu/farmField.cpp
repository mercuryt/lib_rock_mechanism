#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/space/space.h"
#include "../../engine/definitions/plantSpecies.h"
#include <mutex>
void ContextMenu::drawFarmFieldControls(const Point3D& point)
{
	const FactionId& faction = m_window.getFaction();
	if(!faction.exists())
		return;
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	// Farm
	if(space.farm_contains(point, faction))
	{
		auto farmLabel = tgui::Label::create("farm");
		m_root.add(farmLabel);
		farmLabel->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		farmLabel->onMouseEnter([this, point, &space, faction, &area]{
			auto& submenu = makeSubmenu(0);
			auto shrinkButton = tgui::Button::create("shrink");
			shrinkButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(shrinkButton);
			shrinkButton->onClick([this, &space, faction]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				space.farm_remove(m_window.getSelectedBlocks(), faction);
				hide();
			});
			auto setFarmSpeciesButton = tgui::Button::create("set species");
			setFarmSpeciesButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(setFarmSpeciesButton);
			setFarmSpeciesButton->onClick([this, point, faction, &space, &area]{
				auto& submenu2 = makeSubmenu(1);
				auto plantSpeciesUI = widgetUtil::makePlantSpeciesSelectUI(area, point);
				submenu2.add(plantSpeciesUI);
				plantSpeciesUI->onItemSelect([this, point, faction, &space, &area](const tgui::String& string)
				{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					const PlantSpeciesId& species = PlantSpecies::byName(string.toStdString());
					FarmField& field = *space.farm_get(point, faction);
					if(species != field.m_plantSpecies)
					{
						auto& hasFarmFields = area.m_hasFarmFields.getForFaction(faction);
						if(field.m_plantSpecies.exists())
							hasFarmFields.clearSpecies(area, field);
						hasFarmFields.setSpecies(area, field, species);
						hide();
					}

				});
			});
		});
	}
	else if(point.z() != 0)
	{
		const Point3D& below = point.below();
		if(space.plant_anythingCanGrowHereEver(below))
		{
			auto create = [this, point, faction, &area](const PlantSpeciesId& plantSpecies){
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				if(m_window.getSelectedBlocks().empty())
					m_window.selectBlock(point);
				if(!m_window.getArea()->m_hasFarmFields.contains(faction))
					m_window.getArea()->m_hasFarmFields.registerFaction(faction);
				auto& fieldsForFaction = area.m_hasFarmFields.getForFaction(faction);
				FarmField& field = fieldsForFaction.create(area, m_window.getSelectedBlocks());
				fieldsForFaction.setSpecies(area, field, plantSpecies);
				hide();
			};
			auto createButton = tgui::Button::create("create farm field");
			createButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			m_root.add(createButton);
			createButton->onMouseEnter([this, &area, point, create]{
				auto& submenu = makeSubmenu(0);
				auto speciesLabel = tgui::Label::create("species");
				speciesLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
				submenu.add(speciesLabel);
				auto plantSpeciesUI = widgetUtil::makePlantSpeciesSelectUI(area, point);
				submenu.add(plantSpeciesUI);
				plantSpeciesUI->onItemSelect([create](const tgui::String name){
					create(PlantSpecies::byName(name.toStdString()));
				});
			});
			createButton->onClick([create]{ create(widgetUtil::lastSelectedPlantSpecies); });
		}
	}
}
