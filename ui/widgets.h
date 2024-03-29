#pragma once
#include <TGUI/TGUI.hpp>
#include "../engine/datetime.h"
class Area;
class Simulation;
class Plant;
class Block;
struct BlockFeatureType;
struct PlantSpecies;
struct MaterialType;
struct AnimalSpecies;
struct FluidType;
struct ItemType;
struct Faction;
struct MaterialTypeCategory;
struct CraftStepTypeCategory;
struct DateTimeUI final
{
	tgui::SpinControl::Ptr m_hours;
	tgui::SpinControl::Ptr m_days;
	tgui::SpinControl::Ptr m_years;
	tgui::Grid::Ptr m_widget;
	DateTimeUI();
	DateTimeUI(uint8_t hours, uint16_t days, uint16_t years);
	DateTimeUI(DateTime&& dateTime);
	DateTimeUI(Step steps);
	void set(DateTime& dateTime);
	void set(DateTime&& dateTime);
	Step get() const;
};

class AreaSelectUI final
{
	Simulation* m_simulation;
public:
	tgui::ComboBox::Ptr m_widget;
	AreaSelectUI();
	void populate(Simulation& simulation);
	void set(Area& area);
	Area& get() const;
	bool hasSelection() const;
};

namespace widgetUtil
{
	inline const PlantSpecies* lastSelectedPlantSpecies = nullptr;
	tgui::ComboBox::Ptr makePlantSpeciesSelectUI(Block* block);
	inline const MaterialType* lastSelectedMaterial = nullptr;
	tgui::ComboBox::Ptr makeMaterialSelectUI(std::wstring nullLabel = L"");
	inline const MaterialTypeCategory* lastSelectedMaterialCategory = nullptr;
	tgui::ComboBox::Ptr makeMaterialCategorySelectUI(std::wstring nullLabel = L"");
	inline const AnimalSpecies* lastSelectedAnimalSpecies = nullptr;
	tgui::ComboBox::Ptr makeAnimalSpeciesSelectUI();
	inline const FluidType* lastSelectedFluidType = nullptr;
	tgui::ComboBox::Ptr makeFluidTypeSelectUI();
	inline const ItemType* lastSelectedItemType = nullptr;
	tgui::ComboBox::Ptr makeItemTypeSelectUI();
	inline const Faction* lastSelectedFaction = nullptr;
	tgui::ComboBox::Ptr makeFactionSelectUI(Simulation& simulation, std::wstring nullLabel = L"");
	inline const BlockFeatureType* lastSelectedBlockFeatureType = nullptr;
	tgui::ComboBox::Ptr makeBlockFeatureTypeSelectUI();
	inline const CraftStepTypeCategory* lastSelectedCraftStepTypeCategory = nullptr;
	tgui::ComboBox::Ptr makeCraftStepTypeCategorySelectUI();

	void setPadding(tgui::Widget::Ptr wigdet);
}
