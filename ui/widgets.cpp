#include "widgets.h"
#include "pointFeature.h"
#include "displayData.h"
#include "../engine/simulation/simulation.h"
#include "../engine/simulation/hasAreas.h"
#include "../engine/area/area.h"
#include "../engine/definitions/animalSpecies.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <cstdint>
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
	m_years->setMaximum(INT64_MAX / Config::stepsPerYear.get());
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
	for(const auto& pair : simulation.m_hasAreas->getAll())
	{
		const Area& area = *pair.second;
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
	std::wstring name = m_widget->getSelectedItem().toWideString();
	auto found = m_simulation->m_hasAreas->getAll().findIf([&](const Area& area){ return area.m_name == name; });
	assert(found != m_simulation->m_hasAreas->getAll().end());
	return found.second();
}
bool AreaSelectUI::hasSelection() const { return !m_widget->getSelectedItemId().empty(); }
// Plant SpeciesSelector
tgui::ComboBox::Ptr widgetUtil::makePlantSpeciesSelectUI(Area& area, const Point3D& point)
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([&](const tgui::String name){ lastSelectedPlantSpecies = PlantSpecies::byName(name.toWideString()); });
	Space& space =  area.getSpace();
	for(PlantSpeciesId id = PlantSpeciesId::create(0); id < PlantSpecies::size(); ++id)
	{
		if(point.exists() && !space.plant_canGrowHereEver(point, id))
			continue;
		std::wstring name = PlantSpecies::getName(id);
		output->addItem(name, name);
		if(lastSelectedPlantSpecies.exists() && lastSelectedPlantSpecies == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedPlantSpecies.empty() && !selected)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
	}
	return output;
}
// MaterialTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeMaterialSelectUI(MaterialTypeId& lastSelected, std::wstring nullLabel, std::function<bool(const MaterialTypeId&)> predicate)
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	bool selected = false;
	output->onItemSelect([nullLabel, &lastSelected](const tgui::String name){
		assert(!name.empty());
		if(name == nullLabel)
			lastSelected.clear();
		else
			lastSelected = MaterialType::byName(name.toWideString());
	});
	if(!nullLabel.empty())
	{
		output->addItem(nullLabel, nullLabel);
		if(lastSelected.empty())
		{
			output->setSelectedItemById(nullLabel);
			selected = true;
		}
	}
	for(MaterialTypeId id = MaterialTypeId::create(0); id < MaterialType::size(); ++id)
	{
		if(predicate && !predicate(id))
			continue;
		std::wstring name = MaterialType::getName(id);
		output->addItem(name, name);
		if(lastSelected.exists() && lastSelected== id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelected.empty() && !selected)
		{
			output->setSelectedItemById(name);
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
			lastSelectedMaterialCategory.clear();
		else
			lastSelectedMaterialCategory = MaterialTypeCategory::byName(name.toWideString());
	});
	if(!nullLabel.empty())
	{
		output->addItem(nullLabel, nullLabel);
		if(lastSelectedMaterialCategory.empty())
		{
			output->setSelectedItemById(nullLabel);
			selected = true;
		}
	}
	for(MaterialCategoryTypeId id = MaterialCategoryTypeId::create(0); id < MaterialTypeCategory::size(); ++id)
	{
		std::wstring name = MaterialTypeCategory::getName(id);
		output->addItem(name, name);
		if(lastSelectedMaterialCategory.exists() && lastSelectedMaterialCategory == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedMaterialCategory.empty() && !selected)
		{
			output->setSelectedItemById(name);
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
	output->onItemSelect([](const tgui::String name){ lastSelectedAnimalSpecies = AnimalSpecies::byName(name.toWideString()); });
	for(AnimalSpeciesId id = AnimalSpeciesId::create(0); id < AnimalSpecies::size(); ++id)
	{
		std::wstring name = AnimalSpecies::getName(id);
		output->addItem(name, name);
		if(lastSelectedAnimalSpecies.exists() && lastSelectedAnimalSpecies == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedAnimalSpecies.empty() && !selected)
		{
			output->setSelectedItemById(name);
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
	output->onItemSelect([](const tgui::String name){ lastSelectedFluidType = FluidType::byName(name.toWideString()); });
	for(FluidTypeId id = FluidTypeId::create(0); id < FluidType::size(); ++id)
	{
		std::wstring name = FluidType::getName(id);
		output->addItem(name, name);
		if(lastSelectedFluidType.exists() && lastSelectedFluidType == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedFluidType.empty() && !selected)
		{
			output->setSelectedItemById(name);
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
	output->onItemSelect([](const tgui::String name){ lastSelectedItemType = ItemType::byName(name.toWideString()); });
	for(ItemTypeId id = ItemTypeId::create(0); id < ItemType::size(); ++id)
	{
		std::wstring name = ItemType::getName(id);
		output->addItem(name, name);
		if(lastSelectedItemType.exists() && lastSelectedItemType == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedItemType.empty() && !selected)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
	}
	return output;
}
// ItemType and MaterialType or MaterialTypeCategory
std::tuple<tgui::ComboBox::Ptr, tgui::ComboBox::Ptr, tgui::ComboBox::Ptr> widgetUtil::makeItemTypeAndMaterialTypeOrMaterialTypeCategoryUI()
{
	// ItemType.
	auto itemTypeSelectUI = tgui::ComboBox::create();
	for(ItemTypeId id = ItemTypeId::create(0); id < ItemType::size(); ++id)
	{
		std::wstring name = ItemType::getName(id);
		itemTypeSelectUI->addItem(name, name);
	}
	if(lastSelectedItemType.exists())
		itemTypeSelectUI->setSelectedItem(ItemType::getName(lastSelectedItemType));
	else
		itemTypeSelectUI->setSelectedItemByIndex(0);
	lastSelectedItemType = ItemType::byName(itemTypeSelectUI->getSelectedItem().toWideString());
	// MaterialType.
	auto materialTypeSelectUI = tgui::ComboBox::create();
	materialTypeSelectUI->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	materialTypeSelectUI->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	auto materialTypeCategorySelectUI = tgui::ComboBox::create();
	materialTypeCategorySelectUI->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	materialTypeCategorySelectUI->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	auto populateMaterialTypeAndMaterialTypeCategory = [materialTypeSelectUI, materialTypeCategorySelectUI] () {
		const ItemTypeId& itemType = lastSelectedItemType;
		// MaterialType.
		materialTypeSelectUI->removeAllItems();
		materialTypeSelectUI->addItem("any", "");
		for(MaterialTypeId id = MaterialTypeId::create(0); id < MaterialType::size(); ++id)
		{
			std::wstring name = MaterialType::getName(id);
			const auto& materialTypeCategories = ItemType::getMaterialTypeCategories(itemType);
			if(materialTypeCategories.empty())
			{
				materialTypeSelectUI->addItem(name, name);
				continue;
			}
			const MaterialCategoryTypeId& categoryOfMaterialType = MaterialType::getMaterialTypeCategory(id);
			for(const MaterialCategoryTypeId& materialTypeCategory : materialTypeCategories)
				if(materialTypeCategory == categoryOfMaterialType)
				{
					materialTypeSelectUI->addItem(name, name);
					break;
				}
		}
		if(lastSelectedMaterial.exists())
		{
			std::wstring lastMaterialName = MaterialType::getName(lastSelectedMaterial);
			if(materialTypeSelectUI->contains(lastMaterialName))
				materialTypeSelectUI->setSelectedItem(lastMaterialName);
		}
		else
		{
			materialTypeSelectUI->setSelectedItemByIndex(0);
			lastSelectedMaterial.clear();
		}
		// Category.
		materialTypeCategorySelectUI->removeAllItems();
		materialTypeCategorySelectUI->addItem("any", "");
		const auto& materialTypeCategoriesForItemType = ItemType::getMaterialTypeCategories(itemType);
		if(!materialTypeCategoriesForItemType.empty())
		{
			for(const MaterialCategoryTypeId& id : materialTypeCategoriesForItemType)
			{
				std::wstring name = MaterialTypeCategory::getName(id);
				materialTypeCategorySelectUI->addItem(name, name);
			}
			if(lastSelectedMaterialCategory.exists())
			{
				std::wstring name = MaterialTypeCategory::getName(lastSelectedMaterialCategory);
				if(materialTypeCategorySelectUI->contains(name))
					materialTypeCategorySelectUI->setSelectedItem(name);
			}
			else
			{
				materialTypeCategorySelectUI->setSelectedItemByIndex(0);
				lastSelectedMaterialCategory.clear();
			}
		}
	};
	populateMaterialTypeAndMaterialTypeCategory();
	itemTypeSelectUI->onItemSelect([itemTypeSelectUI, populateMaterialTypeAndMaterialTypeCategory]{
		const std::wstring selectedId = itemTypeSelectUI->getSelectedItemId().toWideString();
		if(selectedId.empty())
			return;
		const ItemTypeId& itemType = ItemType::byName(selectedId);
		lastSelectedItemType = itemType;
		populateMaterialTypeAndMaterialTypeCategory();
	});
	materialTypeSelectUI->onItemSelect([materialTypeCategorySelectUI](const tgui::String selected){
		if(selected == "any")
			lastSelectedMaterial.clear();
		else
		{;
			const MaterialTypeId& materialType = MaterialType::byName(selected.toWideString());
			lastSelectedMaterial = materialType;
			materialTypeCategorySelectUI->setSelectedItemByIndex(0);
		}
	});
	materialTypeCategorySelectUI->onItemSelect([materialTypeSelectUI](const tgui::String selected){
		if(selected == "any")
			lastSelectedMaterialCategory.clear();
		else
		{;
			const MaterialCategoryTypeId& materialTypeCategory = MaterialTypeCategory::byName(selected.toWideString());
			lastSelectedMaterialCategory = materialTypeCategory;
			materialTypeSelectUI->setSelectedItemByIndex(0);
		}
	});
	return {itemTypeSelectUI, materialTypeSelectUI, materialTypeCategorySelectUI};
}
// CraftJobType and MaterialType
std::pair<tgui::ComboBox::Ptr, tgui::ComboBox::Ptr> widgetUtil::makeCraftJobTypeAndMaterialTypeUI()
{
	auto craftJobTypeSelectUI = tgui::ComboBox::create();
	for(auto craftJobTypeId = CraftJobTypeId::create(0); craftJobTypeId < CraftJobType::size(); ++craftJobTypeId)
	{
		std::wstring name = CraftJobType::getName(craftJobTypeId);
		craftJobTypeSelectUI->addItem(name, name);
	}
	if(lastSelectedCraftJobType.exists())
		craftJobTypeSelectUI->setSelectedItem(CraftJobType::getName(lastSelectedCraftJobType));
	else
		craftJobTypeSelectUI->setSelectedItemByIndex(0);
	lastSelectedCraftJobType = CraftJobType::byName(craftJobTypeSelectUI->getSelectedItem().toWideString());
	auto materialTypeSelectUI = tgui::ComboBox::create();
	materialTypeSelectUI->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	materialTypeSelectUI->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	auto populateMaterialType = [materialTypeSelectUI]{
		materialTypeSelectUI->removeAllItems();
		materialTypeSelectUI->addItem("any", "");
		const CraftJobTypeId craftJobType = lastSelectedCraftJobType;
		for(auto id = MaterialTypeId::create(0); id < MaterialType::size(); ++id)
			if(CraftJobType::getMaterialTypeCategory(craftJobType) == MaterialType::getMaterialTypeCategory(id))
			{
				std::wstring name = MaterialType::getName(id);
				materialTypeSelectUI->addItem(name, name);
			}
		if(lastSelectedMaterial.exists() && materialTypeSelectUI->contains(MaterialType::getName(lastSelectedMaterial)))
			materialTypeSelectUI->setSelectedItem(MaterialType::getName(lastSelectedMaterial));
		else
		{
			materialTypeSelectUI->setSelectedItemByIndex(0);
			lastSelectedMaterial.clear();
		}
	};
	populateMaterialType();
	craftJobTypeSelectUI->onItemSelect([craftJobTypeSelectUI, populateMaterialType]{
		const std::wstring selectedName = craftJobTypeSelectUI->getSelectedItemId().toWideString();
		if(selectedName.empty())
			return;
		const CraftJobTypeId& craftJobType = CraftJobType::byName(selectedName);
		lastSelectedCraftJobType = craftJobType;
		populateMaterialType();
	});
	materialTypeSelectUI->onItemSelect([](const tgui::String selected){
		if(selected == "any")
			lastSelectedMaterial.clear();
		else
		{;
			const MaterialTypeId& materialType = MaterialType::byName(selected.toWideString());
			lastSelectedMaterial = materialType;
		}
	});
	return {craftJobTypeSelectUI, materialTypeSelectUI};
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
			lastSelectedFaction.clear();
		else
			lastSelectedFaction = simulation.m_hasFactions.byName(id.toWideString());
	});
	if(!nullLabel.empty())
	{
		output->addItem(nullLabel, nullLabel);
		output->setSelectedItemById(nullLabel);
		selected = true;
	}
	for(Faction& faction : simulation.m_hasFactions.getAll())
	{
		output->addItem(faction.name, faction.name);
		if(lastSelectedFaction.exists() && lastSelectedFaction == faction.id)
		{
			output->setSelectedItemById(faction.name);
			selected = true;
		}
		else if(lastSelectedFaction.empty() && !selected)
		{
			output->setSelectedItemById(faction.name);
			selected = true;
		}
	}
	return output;
}
// PointFeatureTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makePointFeatureTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	output->onItemSelect([](const tgui::String name){ lastSelectedPointFeatureType = &PointFeatureType::byName(name.toWideString()); });
	bool selected = false;
	std::vector<PointFeatureType*> types = PointFeatureType::getAll();
	for(const PointFeatureType* itemType : sortByName(types))
	{
		output->addItem(itemType->name, itemType->name);
		if(lastSelectedPointFeatureType && lastSelectedPointFeatureType == itemType)
		{
			output->setSelectedItemById(itemType->name);
			selected = true;
		}
		else if(!lastSelectedPointFeatureType && !selected)
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
	output->onItemSelect([](const tgui::String name){ lastSelectedCraftStepTypeCategory = CraftStepTypeCategory::byName(name.toWideString()); });
	bool selected = false;
	for(auto id = CraftStepTypeCategoryId::create(0); id < CraftStepTypeCategory::size(); ++id)
	{
		std::wstring name = CraftStepTypeCategory::getName(id);
		output->addItem(name, name);
		if(lastSelectedCraftStepTypeCategory.exists() && lastSelectedCraftStepTypeCategory == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedCraftStepTypeCategory.empty() && !selected)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
	}
	return output;
}
// CraftJobTypeSelectUI
tgui::ComboBox::Ptr widgetUtil::makeCraftJobTypeSelectUI()
{
	tgui::ComboBox::Ptr output = tgui::ComboBox::create();
	output->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	output->setExpandDirection(tgui::ComboBox::ExpandDirection::Automatic);
	output->onItemSelect([](const tgui::String name){
		if(!name.empty())
			lastSelectedCraftJobType = CraftJobType::byName(name.toWideString());
	});
	bool selected = false;
	for(auto id = CraftJobTypeId::create(0); id < CraftJobType::size(); ++ id)
	{
		std::wstring name = CraftJobType::getName(id);
		output->addItem(name, name);
		if(lastSelectedCraftJobType.exists() && lastSelectedCraftJobType == id)
		{
			output->setSelectedItemById(name);
			selected = true;
		}
		else if(lastSelectedCraftJobType.empty() && !selected)
		{
			output->setSelectedItemById(name);
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
