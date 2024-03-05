#include "infoPopup.h"
#include "window.h"
#include "../engine/block.h"
#include "../engine/item.h"
#include "../engine/actor.h"
#include "../engine/util.h"
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
void InfoPopup::display(Block& block)
{
	makeWindow();
	if(block.isSolid())
		m_childWindow->setTitle("solid " + block.getSolidMaterial().name);
	else
	{
		std::string title;
		if(block.m_hasBlockFeatures.empty())
		{
			if(block.getBlockBelow() && block.getBlockBelow()->isSolid())
				title = "rough " + block.getBlockBelow()->getSolidMaterial().name + " floor";
			else
				title = "empty space";
		}
		else
		{
			const BlockFeature& blockFeature = block.m_hasBlockFeatures.get().front();
			title = blockFeature.blockFeatureType->name;
		}
		m_childWindow->setTitle(std::move(title));
		for(const BlockFeature& blockFeature : block.m_hasBlockFeatures.get())
			add(tgui::Label::create(blockFeature.blockFeatureType->name + " " + blockFeature.materialType->name));
		for(Actor* actor : block.m_hasActors.getAll())
		{
			auto button = tgui::Button::create(actor->m_name);
			button->onClick([=, this]{ display(*actor); });
			add(button);
		}
		for(Item* item : block.m_hasItems.getAll())
		{
			tgui::Button::Ptr button;
			button = tgui::Button::create(m_window.displayNameForItem(*item));
			button->onClick([=, this]{ display(*item); });
			add(button);
		}
		if(block.m_hasPlant.exists())
		{
			Plant& plant = block.m_hasPlant.get();
			auto button = tgui::Button::create(plant.m_plantSpecies.name);
			button->onClick([&]{ display(plant); });
			add(button);
		}
		for(auto [fluidType, pair] : block.m_fluids)
		{
			Volume volume = pair.first;
			auto label = tgui::Label::create(fluidType->name + " : " + std::to_string(volume));
			add(label);
		}
		if(m_window.getArea()->m_fluidSources.contains(block))
		{
			const FluidSource& source = m_window.getArea()->m_fluidSources.at(block);
			auto label = tgui::Label::create(source.fluidType->name + " source : " + std::to_string(source.level));
			add(label);
		}
		if(m_window.getFaction() && block.m_isPartOfFarmField.contains(*m_window.getFaction()))
		{
			FarmField& field = *block.m_isPartOfFarmField.get(*m_window.getFaction());
			auto label = tgui::Label::create((field.plantSpecies ? field.plantSpecies->name + " " : "") + "field");
			add(label);
		}
	}
}
void InfoPopup::display(Item& item)
{
	makeWindow();
	m_childWindow->setTitle(m_window.displayNameForItem(item));
	if(item.m_itemType.generic)
	{
		add(tgui::Label::create(L"quantity: " + std::to_wstring(item.getQuantity())));
	}
	else
	{
		add(tgui::Label::create(L"quality: " + std::to_wstring(item.m_quality)));
		add(tgui::Label::create(L"wear: " + std::to_wstring(item.m_percentWear) + L"%"));
	}
	for(Item* item : item.m_hasCargo.getItems())
	{
		auto button = tgui::Button::create(m_window.displayNameForItem(*item));
		button->onClick([=, this]{ display(*item); });
		add(button);
	}
	for(Actor* actor : item.m_hasCargo.getActors())
	{
		auto button = tgui::Button::create(actor->m_name);
		button->onClick([=, this]{ display(*actor); });
		add(button);
	}
}
void InfoPopup::display(Actor& actor)
{
	makeWindow();
	m_childWindow->setTitle(actor.m_name);
	std::wstring descriptionText;
	add(tgui::Label::create(util::stringToWideString(actor.m_species.name)));
	add(tgui::Label::create(L"age: " + std::to_wstring(actor.getAgeInYears())));

	for(Item* item : actor.m_equipmentSet.getAll())
	{
		auto button = tgui::Button::create(m_window.displayNameForItem(*item));
		button->onClick([=, this]{ m_window.selectItem(*item); });
		add(button);
	}
	auto more = tgui::Button::create("more");
	add(more);
	more->onClick([this, &actor]{ m_window.showActorDetail(actor); });
	if(m_window.m_editMode || (m_window.getFaction() && m_window.getFaction() == actor.getFaction()))
	{
		auto priorities = tgui::Button::create("priorities");
		add(priorities);
		priorities->onClick([this, &actor]{ m_window.showObjectivePriority(actor); });
	}
}
void InfoPopup::display(Plant& plant)
{
	makeWindow();
	m_childWindow->setTitle(plant.m_plantSpecies.name);
	add(tgui::Label::create(L"percent grown: " + std::to_wstring(plant.getGrowthPercent())));
	add(tgui::Label::create(L"percent foliage: " + std::to_wstring(plant.getPercentFoliage())));
}
void InfoPopup::hide() { m_childWindow->close(); }
bool InfoPopup::isVisible() const { return m_childWindow && m_childWindow->isVisible(); }
