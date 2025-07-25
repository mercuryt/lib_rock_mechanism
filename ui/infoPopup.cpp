#include "infoPopup.h"
#include "window.h"
#include "../engine/space/space.h"
#include "../engine/items/items.h"
#include "../engine/actors/actors.h"
#include "../engine/plants.h"
#include "../engine/util.h"
#include "../engine/definitions/animalSpecies.h"
#include <TGUI/Event.hpp>
#include <TGUI/Layout.hpp>
#include <TGUI/Widget.hpp>
#include <TGUI/Widgets/ChildWindow.hpp>
#include <TGUI/Widgets/HorizontalLayout.hpp>
#include <istream>
#include <new>
#include <string>
InfoPopup::InfoPopup(Window& window) : m_window(window) { }
void InfoPopup::makeWindow()
{
	if(m_childWindow)
		m_childWindow->close();
	m_count = 0;
	m_childWindow = tgui::ChildWindow::create();
	m_window.getGameOverlay().getGroup()->add(m_childWindow);
	//m_m_childWindow->setSize(tgui::bindWidth(m_grid) + 4, tgui::bindHeight(m_grid) + 4);
	//m_grid->setPosition(2,2);
	m_childWindow->setKeepInParent(true);
	m_grid = tgui::Grid::create();
	m_childWindow->add(m_grid);
}
void InfoPopup::add(tgui::Widget::Ptr widget) { m_grid->addWidget(widget, ++m_count, 1); }
void InfoPopup::display(const Point3D& point)
{
	makeWindow();
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	Actors& actors = area.getActors();
	if(space.solid_is(point))
		m_childWindow->setTitle(L"solid " + MaterialType::getName(space.solid_get(point)));
	else
	{
		std::wstring title;
		auto& pointFeatures = space.pointFeature_getAll(point);
		if(pointFeatures.empty())
		{
			const Point3D& below = point.below();
			if(below.exists() && space.solid_is(below))
				title = L"rough " + MaterialType::getName(space.solid_get(below)) + L" floor";
			else
				title = L"empty space";
		}
		else
		{
			const PointFeature& pointFeature = pointFeatures.front();
			title = pointFeature.pointFeatureType->name;
		}
		m_childWindow->setTitle(std::move(title));
		for(const PointFeature& pointFeature : pointFeatures)
			add(tgui::Label::create(pointFeature.pointFeatureType->name + L" " + MaterialType::getName(pointFeature.materialType)));
		add(tgui::Label::create(L"temperature: " + std::to_wstring(space.temperature_get(point).get())));
		for(const ActorIndex& actor : space.actor_getAll(point))
		{
			auto button = tgui::Button::create(actors.getName(actor));
			button->onClick([=, this]{ display(actor); });
			add(button);
		}
		for(const ItemIndex& item : space.item_getAll(point))
		{
			tgui::Button::Ptr button;
			button = tgui::Button::create(m_window.displayNameForItem(item));
			button->onClick([=, this]{ display(item); });
			add(button);
		}
		if(space.plant_exists(point))
		{
			const PlantIndex& plant = space.plant_get(point);
			const Plants& plants = area.getPlants();
			auto button = tgui::Button::create(PlantSpecies::getName(plants.getSpecies(plant)));
			button->onClick([&]{ display(plant); });
			add(button);
		}
		for(const FluidData& fluidData : space.fluid_getAll(point))
		{
			auto label = tgui::Label::create(FluidType::getName(fluidData.type) + L" : " + std::to_wstring(fluidData.volume.get()));
			add(label);
		}
		if(m_window.getArea()->m_fluidSources.contains(point))
		{
			const FluidSource& source = m_window.getArea()->m_fluidSources.at(point);
			auto label = tgui::Label::create(FluidType::getName(source.fluidType) + L" source : " + std::to_wstring(source.level.get()));
			add(label);
		}
		if(m_window.getFaction().exists() && space.farm_contains(point, m_window.getFaction()))
		{
			const FarmField& field = *space.farm_get(point, m_window.getFaction());
			auto label = tgui::Label::create((field.plantSpecies.exists() ? PlantSpecies::getName(field.plantSpecies) + L" " : L"") + L"field");
			add(label);
		}
	}
	m_update = [this, point]{ display(point); };
}
void InfoPopup::display(const ItemIndex& item)
{
	makeWindow();
	m_childWindow->setTitle(m_window.displayNameForItem(item));
	Area& area = *m_window.getArea();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	if(ItemType::getIsGeneric(items.getItemType(item)))
	{
		add(tgui::Label::create(L"quantity: " + std::to_wstring(items.getQuantity(item).get())));
	}
	else
	{
		add(tgui::Label::create(L"quality: " + std::to_wstring(items.getQuality(item).get())));
		add(tgui::Label::create(L"wear: " + std::to_wstring(items.getWear(item).get()) + L"%"));
	}
	if(items.cargo_exists(item))
	{
		for(const ItemIndex& cargoItem : items.cargo_getItems(item))
		{
			auto button = tgui::Button::create(m_window.displayNameForItem(cargoItem));
			button->onClick([=, this]{ display(cargoItem); });
			add(button);
			if(m_window.m_editMode)
			{
				auto destroy = tgui::Button::create("destory");
				add(destroy);
				destroy->onClick([this, cargoItem, item, &items]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					items.cargo_removeItem(item, cargoItem);
					items.destroy(cargoItem);
					m_update();
				});
			}
		}
		for(const ActorIndex& actor : items.cargo_getActors(item))
		{
			auto button = tgui::Button::create(actors.getName(actor));
			button->onClick([=, this]{ display(actor); });
			add(button);
		}
		m_update = [this, item]{ display(item); };
	}
}
void InfoPopup::display(const ActorIndex& actor)
{
	Area& area = *m_window.getArea();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	makeWindow();
	m_childWindow->setTitle(actors.getName(actor));
	std::wstring descriptionText;
	add(tgui::Label::create(AnimalSpecies::getName(actors.getSpecies(actor))));
	add(tgui::Label::create(L"age: " + std::to_wstring(actors.getAgeInYears(actor).get())));
	add(tgui::Label::create(L"action: " + actors.getActionDescription(actor)));
	for(const ItemReference& itemRef : actors.equipment_getAll(actor))
	{
		const ItemIndex item = itemRef.getIndex(items.m_referenceData);
		auto button = tgui::Button::create(m_window.displayNameForItem(item));
		button->onClick([=, this]{ m_window.selectItem(item); });
		add(button);
	}
	auto more = tgui::Button::create("more");
	add(more);
	more->onClick([this, actor]{ m_window.showActorDetail(actor); });
	if(actors.isSentient(actor) && (m_window.m_editMode || (m_window.getFaction().exists() && m_window.getFaction() == actors.getFaction(actor).exists())))
	{
		auto priorities = tgui::Button::create("priorities");
		add(priorities);
		priorities->onClick([this, actor]{ m_window.showObjectivePriority(actor); });
	}
	if(!actors.sleep_isAwake(actor))
		add(tgui::Label::create("sleeping"));
	else
	{
		Percent tiredPercent = actors.sleep_getPercentTired(actor);
		if(actors.sleep_isTired(actor))
			tiredPercent += 100;
		add(tgui::Label::create(std::to_wstring(tiredPercent.get()) + L" % tired"));
	}
	Percent hungerPercent = actors.eat_getPercentStarved(actor);
	if(actors.eat_isHungry(actor))
		hungerPercent += 100;
	add(tgui::Label::create(std::to_wstring(hungerPercent.get()) + L" % hunger"));
	Percent thirstPercent = actors.drink_getPercentDead(actor);
	if(actors.drink_isThirsty(actor))
		thirstPercent += 100;
	add(tgui::Label::create(std::to_wstring(thirstPercent.get()) + L" % thirst"));
	m_update = [this, actor]{ display(actor); };
	m_childWindow->setFocused(false);
}
void InfoPopup::display(const PlantIndex& plant)
{
	Area& area = *m_window.getArea();
	Plants& plants = area.getPlants();
	makeWindow();
	m_childWindow->setTitle(PlantSpecies::getName(plants.getSpecies(plant)));
	add(tgui::Label::create(L"percent grown: " + std::to_wstring(plants.getPercentGrown(plant).get())));
	add(tgui::Label::create(L"percent foliage: " + std::to_wstring(plants.getPercentFoliage(plant).get())));
	if(plants.getVolumeFluidRequested(plant) != 0)
	{
		Percent thirstPercent = plants.getFluidEventPercentComplete(plant);
		add(tgui::Label::create(std::to_wstring(thirstPercent.get()) + L" % thirst"));
	}
	m_update = [this, plant]{ display(plant); };
}
void InfoPopup::hide() { m_childWindow->close(); m_childWindow = nullptr; }
bool InfoPopup::isVisible() const { return m_childWindow && m_childWindow->isVisible(); }
