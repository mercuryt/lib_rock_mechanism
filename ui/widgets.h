#pragma once
#include <TGUI/TGUI.hpp>
#include "../engine/datetime.h"
#include "../engine/plant.h"
#include "animalSpecies.h"
class Area;
class Simulation;
class DateTimeUI final
{
	tgui::SpinControl::Ptr m_hours;
	tgui::SpinControl::Ptr m_days;
	tgui::SpinControl::Ptr m_years;
public:
	tgui::Grid::Ptr m_widget;
	DateTimeUI();
	DateTimeUI(uint8_t hours, uint8_t days, uint8_t years);
	DateTimeUI(DateTime& dateTime) { set(dateTime); }
	void set(DateTime& dateTime);
	void set(DateTime&& dateTime);
	DateTime get() const;
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
	tgui::ComboBox::Ptr makeMaterialSelectUI();
	inline const AnimalSpecies* lastSelectedAnimalSpecies = nullptr;
	tgui::ComboBox::Ptr makeAnimalSpeciesSelectUI();
	inline const FluidType* lastSelectedFluidType = nullptr;
	tgui::ComboBox::Ptr makeFluidTypeSelectUI();
	inline const ItemType* lastSelectedItemType = nullptr;
	tgui::ComboBox::Ptr makeItemTypeSelectUI();
	void setPadding(tgui::Widget::Ptr wigdet);
}