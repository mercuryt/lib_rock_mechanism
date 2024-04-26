#include "widgets.h"
#include "blockFeature.h"
#include "displayData.h"
#include "../engine/simulation.h"
#include "plant.h"
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <cstdint>
template<typename T>
	std::vector<T*> sortByName(std::vector<T*>& input)
	{
		std::vector<T*> output;
		for(auto& item : input)
			output.push_back(item);
		std::ranges::sort(output, std::less{}, &T::name);
		return output;
	}
template<typename T>
	std::vector<T*> sortByName(std::vector<T>& input)
	{
		std::vector<T*> output;
		for(auto& item : input)
			output.push_back(&item);
		std::ranges::sort(output, std::less{}, &T::name);
		return output;
	}
template<typename T>
	std::vector<T*> sortByName(std::list<T>& input)
	{
		std::vector<T*> output;
		for(auto& item : input)
			output.push_back(&item);
		std::ranges::sort(output, std::less{}, &T::name);
		return output;
	}
// DateTime.
DateTimeUI::DateTimeUI(uint8_t hours,uint16_t days, uint16_t years) : m_hours(tgui::SpinControl::create()), m_days(tgui::SpinControl::create()), m_years(tgui::SpinControl::create()), m_widget(tgui::Grid::create())
{
	m_widget->addWidget(tgui::Label::create("hours"), 1, 1);
	m_widget->addWidget(m_hours, 1, 2);
	m_hours->setMinimum(1);
	m_hours->setMaximum(24);
	m_hours->setValue(hours);

	m_widget->addWidget(tgui::Label::create("days"), 2, 1);
	m_widget->addWidget(m_days, 2, 2);
	m_days->setMinimum(1);
	m_days->setMaximum(365);
	m_days->setValue(days);

	m_widget->addWidget(tgui::Label::create("years"), 3, 1);
	m_widget->addWidget(m_years, 3, 2);
	m_years->setMinimum(0);
	m_years->setMaximum(UINT64_MAX / Config::stepsPerYear);
	m_years->setValue(years);
}
DateTimeUI::DateTimeUI() : DateTimeUI(1,1,1) { }
DateTimeUI::DateTimeUI(DateTime&& dateTime) : DateTimeUI(dateTime.hour, dateTime.day, dateTime.year) { }
DateTimeUI::DateTimeUI(Step step) : DateTimeUI(DateTime(step)) { }
void DateTimeUI::set(DateTime& dateTime)
{
	m_hours->setValue(dateTime.hour);
	m_days->setValue(dateTime.day);
	m_years->setValue(dateTime.year);
}
void DateTimeUI::set(DateTime&& dateTime) { set(dateTime); }
Step DateTimeUI::get() const
{
	DateTime dateTime{
		static_cast<uint8_t>(m_hours->getValue()),
	       	static_cast<uint16_t>(m_days->getValue()),
	       	static_cast<uint16_t>(m_years->getValue())
	};
	return dateTime.toSteps();
}
// Area Selector.
AreaSelectUI::AreaSelectUI() : m_widget(tgui::ComboBox::create()) 
{ 
	m_widget->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	m_widget->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
}
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
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([&](const tgui::String name){ lastSelectedPlantSpecies = &PlantSpecies::byName(name.toStdString()); });

	for(const PlantSpecies* plantSpecies : sortByName(plantSpeciesDataStore))
	{
		if(block && !block->m_hasPlant.canGrowHereEver(*plantSpecies))
			continue;
		output->addItem(plantSpecies->name, plantSpecies->name);
		if(lastSelectedPlantSpecies && lastSelectedPlantSpecies == plantSpecies)
		{
			output->setSelectedItemById(plantSpecies->name);
			selected = true;
		}
		else if(!lastSelectedPlantSpecies && !selected)
		{
			output->setSelectedItemById(plantSpecies->name);
			selected = true;
		}
	}
	return output;
}
// MaterialTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeMaterialSelectUI(std::wstring nullLabel, std::function<bool(const MaterialType&)> predicate)
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([nullLabel](const tgui::String name){ 
		assert(!name.empty());
		if(name == nullLabel)
			lastSelectedMaterial = nullptr;
		else
			lastSelectedMaterial = &MaterialType::byName(name.toStdString()); 
	});
	if(!nullLabel.empty())
	{
		output->addItem(nullLabel, nullLabel);
		if(!lastSelectedMaterial)
		{
			output->setSelectedItemById(nullLabel);
			selected = true;
		}
	}
	for(const MaterialType* materialType : sortByName(materialTypeDataStore))
	{
		if(predicate && !predicate(*materialType))
			continue;
		output->addItem(materialType->name, materialType->name);
		if(lastSelectedMaterial && lastSelectedMaterial == materialType)
		{
			output->setSelectedItemById(materialType->name);
			selected = true;
		}
		else if(!lastSelectedMaterial && !selected)
		{
			output->setSelectedItemById(materialType->name);
			selected = true;
		}
	}
	return output;
}
// MaterialTypeCategorySelectUI
tgui::ComboBox::Ptr widgetUtil::makeMaterialCategorySelectUI(std::wstring nullLabel)
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([nullLabel](const tgui::String name){ 
		assert(!name.empty());
		if(name == nullLabel)
			lastSelectedMaterialCategory = nullptr;
		else
			lastSelectedMaterialCategory = &MaterialTypeCategory::byName(name.toStdString()); 
	});
	if(!nullLabel.empty())
	{
		output->addItem(nullLabel, nullLabel);
		if(!lastSelectedMaterialCategory)
		{
			output->setSelectedItemById(nullLabel);
			selected = true;
		}
	}
	for(const MaterialTypeCategory* materialTypeCategory : sortByName(materialTypeCategoryDataStore))
	{
		output->addItem(materialTypeCategory->name, materialTypeCategory->name);
		if(lastSelectedMaterialCategory && lastSelectedMaterialCategory == materialTypeCategory)
		{
			output->setSelectedItemById(materialTypeCategory->name);
			selected = true;
		}
		else if(!lastSelectedMaterialCategory && !selected)
		{
			output->setSelectedItemById(materialTypeCategory->name);
			selected = true;
		}
	}
	return output;
}
// AnimalSpeciesSelectUI
tgui::ComboBox::Ptr widgetUtil::makeAnimalSpeciesSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([](const tgui::String name){ lastSelectedAnimalSpecies = &AnimalSpecies::byName(name.toStdString()); });
	for(const AnimalSpecies* animalSpecies : sortByName(animalSpeciesDataStore))
	{
		output->addItem(animalSpecies->name, animalSpecies->name);
		if(lastSelectedAnimalSpecies && lastSelectedAnimalSpecies == animalSpecies)
		{
			output->setSelectedItemById(animalSpecies->name);
			selected = true;
		}
		else if(!lastSelectedAnimalSpecies && !selected)
		{
			output->setSelectedItemById(animalSpecies->name);
			selected = true;
		}
	}
	return output;
}
// FluidTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeFluidTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([](const tgui::String name){ lastSelectedFluidType = &FluidType::byName(name.toStdString()); });
	for(const FluidType* fluidType : sortByName(fluidTypeDataStore))
	{
		output->addItem(fluidType->name, fluidType->name);
		if(lastSelectedFluidType && lastSelectedFluidType == fluidType)
		{
			output->setSelectedItemById(fluidType->name);
			selected = true;
		}
		else if(!lastSelectedFluidType && !selected)
		{
			output->setSelectedItemById(fluidType->name);
			selected = true;
		}
	}
	return output;
}
// ItemTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeItemTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([](const tgui::String name){ lastSelectedItemType = &ItemType::byName(name.toStdString()); });
	for(const ItemType* itemType : sortByName(itemTypeDataStore))
	{
		output->addItem(itemType->name, itemType->name);
		if(lastSelectedItemType && lastSelectedItemType == itemType)
		{
			output->setSelectedItemById(itemType->name);
			selected = true;
		}
		else if(!lastSelectedItemType && !selected)
		{
			output->setSelectedItemById(itemType->name);
			selected = true;
		}
	}
	return output;
}
// FactionSelectUI
tgui::ComboBox::Ptr widgetUtil::makeFactionSelectUI(Simulation& simulation, std::wstring nullLabel)
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([output, &simulation, nullLabel]{ 
		auto id = output->getSelectedItemId();
		if(id == nullLabel)
			lastSelectedFaction = nullptr;
		else
			lastSelectedFaction = &static_cast<const Faction&>(simulation.m_hasFactions.byName(id.toWideString())); 
	});
	if(!nullLabel.empty())
	{
		output->addItem(nullLabel, nullLabel);
		output->setSelectedItemById(nullLabel);
		selected = true;
	}
	auto factions = sortByName(simulation.m_hasFactions.getAll());
	for(const Faction* faction : factions)
	{
		output->addItem(faction->name, faction->name);
		if(lastSelectedFaction && lastSelectedFaction == faction)
		{
			output->setSelectedItemById(faction->name);
			selected = true;
		}
		else if(!lastSelectedFaction && !selected)
		{
			output->setSelectedItemById(faction->name);
			selected = true;
		}
	}
	return output;
}
// BlockFeatureTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeBlockFeatureTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	output->onItemSelect([](const tgui::String name){ lastSelectedBlockFeatureType = &BlockFeatureType::byName(name.toStdString()); });
	bool selected = false;
	std::vector<BlockFeatureType*> types = BlockFeatureType::getAll();
	for(const BlockFeatureType* itemType : sortByName(types))
	{
		output->addItem(itemType->name, itemType->name);
		if(lastSelectedBlockFeatureType && lastSelectedBlockFeatureType == itemType)
		{
			output->setSelectedItemById(itemType->name);
			selected = true;
		}
		else if(!lastSelectedBlockFeatureType && !selected)
		{
			output->setSelectedItemById(itemType->name);
			selected = true;
		}
	}
	return output;
}
// CraftStepTypeCategorySelectUI
tgui::ComboBox::Ptr widgetUtil::makeCraftStepTypeCategorySelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	output->onItemSelect([](const tgui::String name){ lastSelectedCraftStepTypeCategory = &CraftStepTypeCategory::byName(name.toStdString()); });
	bool selected = false;
	for(const CraftStepTypeCategory* craftStepTypeCategory : sortByName(craftStepTypeCategoryDataStore))
	{
		output->addItem(craftStepTypeCategory->name, craftStepTypeCategory->name);
		if(lastSelectedCraftStepTypeCategory && lastSelectedCraftStepTypeCategory == craftStepTypeCategory)
		{
			output->setSelectedItemById(craftStepTypeCategory->name);
			selected = true;
		}
		else if(!lastSelectedCraftStepTypeCategory && !selected)
		{
			output->setSelectedItemById(craftStepTypeCategory->name);
			selected = true;
		}
	}
	return output;
}
// Util
void widgetUtil::setPadding(tgui::Widget::Ptr wigdet)
{
	wigdet->setPosition("5%", "5%");
	wigdet->setSize("90%", "90%");
}
