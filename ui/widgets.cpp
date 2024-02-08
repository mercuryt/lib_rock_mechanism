#include "widgets.h"
#include "../engine/simulation.h"
#include "../engine/area.h"
#include "materialType.h"
#include "plant.h"
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <cstdint>
// DateTime.
DateTimeUI::DateTimeUI(uint8_t hours,uint8_t days, uint8_t years) : m_hours(tgui::SpinControl::create()), m_days(tgui::SpinControl::create()), m_years(tgui::SpinControl::create()), m_widget(tgui::Grid::create())
{
	m_widget->addWidget(tgui::Label::create("hours"), 1, 1);
	m_widget->addWidget(m_hours, 1, 2);
	m_hours->setValue(hours);

	m_widget->addWidget(tgui::Label::create("days"), 2, 1);
	m_widget->addWidget(m_days, 2, 2);
	m_days->setValue(days);

	m_widget->addWidget(tgui::Label::create("years"), 3, 1);
	m_widget->addWidget(m_years, 3, 2);
	m_years->setValue(years);
}
DateTimeUI::DateTimeUI() : DateTimeUI(1,1,1) { }
DateTimeUI::DateTimeUI(DateTime& dateTime) : DateTimeUI(dateTime.hour, dateTime.day, dateTime.year) { }
void DateTimeUI::set(DateTime& dateTime)
{
	m_hours->setValue(dateTime.hour);
	m_days->setValue(dateTime.day);
	m_years->setValue(dateTime.year);
}
void DateTimeUI::set(DateTime&& dateTime) { set(dateTime); }
DateTime DateTimeUI::get() const
{
	return {static_cast<uint8_t>(m_hours->getValue()), static_cast<uint16_t>(m_days->getValue()), static_cast<uint16_t>(m_years->getValue())};
}
// Area Selector.
AreaSelectUI::AreaSelectUI() : m_widget(tgui::ComboBox::create()) { }
void AreaSelectUI::populate(Simulation& simulation)
{
	m_widget->removeAllItems();
	std::wstring firstAreaName;
	for(Area& area : simulation.m_areas)
	{
		if(firstAreaName.empty())
			firstAreaName = area.m_name;
		m_widget->addItem(area.m_name, area.m_name);
	}
	if(!firstAreaName.empty())
		m_widget->setSelectedItemById(firstAreaName);
	m_simulation = &simulation;
}
void AreaSelectUI::set(Area& area) { m_widget->setSelectedItem(area.m_name); }
Area& AreaSelectUI::get() const
{
	auto found = std::ranges::find(m_simulation->m_areas, m_widget->getSelectedItemId().toWideString(), &Area::m_name);
	assert(found != m_simulation->m_areas.end());
	return *found;
}
bool AreaSelectUI::hasSelection() const { return !m_widget->getSelectedItemId().empty(); }
// Plant SpeciesSelector
tgui::ComboBox::Ptr widgetUtil::makePlantSpeciesSelectUI(Block* block)
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	bool selected = false;
	for(const PlantSpecies& plantSpecies : PlantSpecies::data)
	{
		if(block && !block->m_hasPlant.canGrowHereEver(plantSpecies))
			continue;
		output->addItem(plantSpecies.name, plantSpecies.name);
		if(lastSelectedPlantSpecies && *lastSelectedPlantSpecies == plantSpecies)
			output->setSelectedItemById(plantSpecies.name);
		else if(!selected)
		{
			output->setSelectedItemById(plantSpecies.name);
			selected = true;
		}
	}
	output->onItemSelect([&](const tgui::String name){ lastSelectedPlantSpecies = &PlantSpecies::byName(name.toStdString()); });
	return output;
}
// MaterialTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeMaterialSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	bool selected = false;
	for(const MaterialType& materialType : MaterialType::data)
	{
		output->addItem(materialType.name, materialType.name);
		if(lastSelectedMaterial && *lastSelectedMaterial == materialType)
			output->setSelectedItemById(materialType.name);
		else if(!selected)
		{
			output->setSelectedItemById(materialType.name);
			selected = true;
		}
	}
	output->onItemSelect([&](const tgui::String name){ lastSelectedMaterial = &MaterialType::byName(name.toStdString()); });
	return output;
}
// AnimalSpeciesSelectUI
tgui::ComboBox::Ptr widgetUtil::makeAnimalSpeciesSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	bool selected = false;
	for(const AnimalSpecies& animalSpecies : AnimalSpecies::data)
	{
		output->addItem(animalSpecies.name, animalSpecies.name);
		if(lastSelectedAnimalSpecies && lastSelectedAnimalSpecies == &animalSpecies)
			output->setSelectedItemById(animalSpecies.name);
		else if(!selected)
		{
			output->setSelectedItemById(animalSpecies.name);
			selected = true;
		}
	}
	output->onItemSelect([&](const tgui::String name){ lastSelectedAnimalSpecies = &AnimalSpecies::byName(name.toStdString()); });
	return output;
}
// FluidTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeFluidTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	bool selected = false;
	for(const FluidType& fluidType : FluidType::data)
	{
		output->addItem(fluidType.name, fluidType.name);
		if(lastSelectedFluidType && lastSelectedFluidType == &fluidType)
			output->setSelectedItemById(fluidType.name);
		else if(!selected)
		{
			output->setSelectedItemById(fluidType.name);
			selected = true;
		}
	}
	output->onItemSelect([&](const tgui::String name){ lastSelectedFluidType = &FluidType::byName(name.toStdString()); });
	return output;
}
// ItemTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeItemTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	bool selected = false;
	for(const ItemType& itemType : ItemType::data)
	{
		output->addItem(itemType.name, itemType.name);
		if(lastSelectedItemType && lastSelectedItemType == &itemType)
			output->setSelectedItemById(itemType.name);
		else if(!selected)
		{
			output->setSelectedItemById(itemType.name);
			selected = true;
		}
	}
	output->onItemSelect([&](const tgui::String name){ lastSelectedItemType = &ItemType::byName(name.toStdString()); });
	return output;
}
// Util
void widgetUtil::setPadding(tgui::Widget::Ptr wigdet)
{
	wigdet->setPosition("5%", "5%");
	wigdet->setSize("90%", "90%");
}
