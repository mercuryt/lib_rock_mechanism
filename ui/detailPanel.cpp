#include "detailPanel.h"
#include "window.h"
#include "../engine/block.h"
#include "../engine/item.h"
#include "../engine/actor.h"
#include "../engine/util.h"
#include <TGUI/Event.hpp>
#include <istream>
#include <new>
#include <string>
DetailPanel::DetailPanel(Window& window, tgui::Group::Ptr gameGui) : m_window(window), m_panel(tgui::Panel::create()), m_name(tgui::Label::create()), m_description(tgui::Label::create()), m_contents(tgui::Group::create()), m_more(tgui::Button::create("more")), m_exit(tgui::Button::create("exit"))
{
	gameGui->add(m_panel);
	m_panel->add(m_name);
	m_panel->add(m_description);
	m_panel->add(m_contents);
	m_panel->add(m_exit);
	m_exit->onClick([this]{ hide(); });
}
void DetailPanel::display(Block& block)
{
	m_contents->removeAllWidgets();
	if(block.isSolid())
	{
		m_name->setVisible(true);
		m_name->setText(block.getSolidMaterial().name);
		m_description->setText("solid");
	}
	else
	{
		m_name->setVisible(false);
		if(block.m_hasBlockFeatures.empty())
			m_description->setText("empty space");
		else
		{
			std::wstring featuresDescription;
			for(const BlockFeature& blockFeature : block.m_hasBlockFeatures.get())
			{
				featuresDescription.append(util::stringToWideString(blockFeature.materialType->name));
				featuresDescription.append(L" ");
				featuresDescription.append(util::stringToWideString(blockFeature.blockFeatureType->name));
				featuresDescription.append(L", ");
			}
			m_description->setText(featuresDescription);
		}
		for(Actor* actor : block.m_hasActors.getAll())
		{
			auto button = tgui::Button::create(actor->m_name);
			button->onClick([&]{ m_window.selectActor(*actor); });
			m_contents->add(button);
		}
		for(Item* item : block.m_hasItems.getAll())
		{
			tgui::Button::Ptr button;
			button = tgui::Button::create(m_window.displayNameForItem(*item));
			button->onClick([&]{ m_window.selectItem(*item); });
			m_contents->add(button);
		}
		if(block.m_hasPlant.exists())
		{
			Plant& plant = block.m_hasPlant.get();
			auto button = tgui::Button::create(plant.m_plantSpecies.name);
			button->onClick([&]{ m_window.selectPlant(plant); });
			m_contents->add(button);
		}
		for(auto [fluidType, pair] : block.m_fluids)
		{
			Volume volume = pair.first;
			auto label = tgui::Label::create(fluidType->name + std::to_string(volume));
			m_contents->add(label);
		}
	}
	m_more->setVisible(false);
	m_panel->setVisible(true);
}
void DetailPanel::display(Item& item)
{
	m_name->setVisible(true);
	m_name->setText(m_window.displayNameForItem(item));
	std::wstring descriptionText;
	if(item.m_itemType.generic)
	{
		descriptionText.append(L"quantity: ");
		descriptionText.append(std::to_wstring(item.getQuantity()));
	}
	else
	{
		descriptionText.append(L"quality: ");
		descriptionText.append(std::to_wstring(item.m_quality));
	}
	m_description->setText(descriptionText);
	m_contents->removeAllWidgets();
	for(Item* item : item.m_hasCargo.getItems())
	{
		auto button = tgui::Button::create(m_window.displayNameForItem(*item));
		button->onClick([&]{ m_window.selectItem(*item); });
		m_contents->add(button);
	}
	for(Actor* actor : item.m_hasCargo.getActors())
	{
		auto button = tgui::Button::create(actor->m_name);
		button->onClick([&]{ m_window.selectActor(*actor); });
		m_contents->add(button);
	}
	m_more->setVisible(false);
	m_panel->setVisible(true);
}
void DetailPanel::display(Actor& actor)
{
	m_name->setVisible(true);
	m_name->setText(actor.m_name);
	std::wstring descriptionText;
	descriptionText.append(util::stringToWideString(actor.m_species.name));
	descriptionText.append(L"age: ");
	descriptionText.append(actor.getAge());
	m_description->setText(descriptionText);
	m_contents->removeAllWidgets();
	for(Item* item : actor.m_equipmentSet.getAll())
	{
		auto button = tgui::Button::create(m_window.displayNameForItem(*item));
		button->onClick([&]{ m_window.selectItem(*item); });
		m_contents->add(button);
	}
	m_more->setVisible(true);
	m_more->onClick([&]{ m_window.detailView(actor); });
	m_panel->setVisible(true);
}
void DetailPanel::display(Plant& plant)
{
	m_name->setVisible(true);
	m_name->setText(plant.m_plantSpecies.name);
	std::wstring descriptionText;
	descriptionText.append(L"age: ");
	descriptionText.append(std::to_wstring(plant.getAge()));
	descriptionText.append(L"percent grown: ");
	descriptionText.append(std::to_wstring(plant.getGrowthPercent()));
	descriptionText.append(L"percent foliage: ");
	descriptionText.append(std::to_wstring(plant.getPercentFoliage()));
	m_description->setText(descriptionText);
	m_contents->setVisible(false);
	m_more->setVisible(false);
	m_panel->setVisible(true);
}
