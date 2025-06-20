#pragma once
#include "../engine/numericTypes/types.h"
#include <TGUI/TGUI.hpp>
class Area;
class Simulation;
class BlockIndex;
struct BlockFeatureType;
struct PlantSpecies;
struct MaterialType;
struct AnimalSpecies;
struct FluidType;
struct ItemType;
struct Faction;
struct MaterialTypeCategory;
struct CraftStepTypeCategory;
struct CraftJobType;
struct DateTime;
struct ItemParamaters;
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
	inline PlantSpeciesId lastSelectedPlantSpecies;
	tgui::ComboBox::Ptr makePlantSpeciesSelectUI(Area& area, const BlockIndex& block);
	inline MaterialTypeId lastSelectedMaterial;
	inline MaterialTypeId lastSelectedConstructionMaterial;
	tgui::ComboBox::Ptr makeMaterialSelectUI(MaterialTypeId& lastSelected, std::wstring nullLabel = L"", std::function<bool(const MaterialTypeId&)> predicate = nullptr);
	inline MaterialCategoryTypeId lastSelectedMaterialCategory;
	tgui::ComboBox::Ptr makeMaterialCategorySelectUI(std::wstring nullLabel = L"");
	inline AnimalSpeciesId lastSelectedAnimalSpecies;
	tgui::ComboBox::Ptr makeAnimalSpeciesSelectUI();
	inline FluidTypeId lastSelectedFluidType;
	tgui::ComboBox::Ptr makeFluidTypeSelectUI();
	inline ItemTypeId lastSelectedItemType;
	tgui::ComboBox::Ptr makeItemTypeSelectUI();
	inline FactionId lastSelectedFaction;
	tgui::ComboBox::Ptr makeFactionSelectUI(Simulation& simulation, std::wstring nullLabel = L"");
	inline const BlockFeatureType* lastSelectedBlockFeatureType = nullptr;
	tgui::ComboBox::Ptr makeBlockFeatureTypeSelectUI();
	inline CraftStepTypeCategoryId lastSelectedCraftStepTypeCategory;
	tgui::ComboBox::Ptr makeCraftStepTypeCategorySelectUI();
	inline CraftJobTypeId lastSelectedCraftJobType;
	tgui::ComboBox::Ptr makeCraftJobTypeSelectUI();
	std::pair<tgui::ComboBox::Ptr, tgui::ComboBox::Ptr> makeCraftJobTypeAndMaterialTypeUI();
	std::tuple<tgui::ComboBox::Ptr, tgui::ComboBox::Ptr, tgui::ComboBox::Ptr> makeItemTypeAndMaterialTypeOrMaterialTypeCategoryUI();
	// In file widgets/item.cpp
	std::array<tgui::Widget::Ptr, 7> makeCreateItemUI(std::function<void(ItemParamaters)> callback);

	void setPadding(tgui::Widget::Ptr wigdet);
}
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
	std::vector<const T*> sortByName(std::vector<const T*>& input)
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
template<typename T>
	std::vector<T*> sortByName(std::unordered_set<T>& input)
	{
		std::vector<T*> output;
		for(auto& item : input)
			output.push_back(&item);
		std::ranges::sort(output, std::less{}, &T::name);
		return output;
	}
