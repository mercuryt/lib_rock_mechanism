#include "contextMenu.h"
#include "window.h"
#include "../engine/actor.h"
#include "../engine/goTo.h"
#include "../engine/station.h"
#include "../engine/installItem.h"
#include "../engine/kill.h"
#include <TGUI/Layout.hpp>
ContextMenu::ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup) : m_window(window), m_rootGui(tgui::Panel::create()), m_gameOverlayGroup(gameOverlayGroup)
{
	m_rootGui->setVisible(false);
	gameOverlayGroup->add(m_rootGui, "rootGui");
}
void ContextMenu::draw(Block& block)
{
	//TODO: shift to add to end of work queue.
	auto actors = m_window.getSelectedActors();
	if(actors.size())
	{
		// Go.
		auto goTo = tgui::Button::create("go to");
		m_rootGui->add(goTo);
		goTo->onClick([&]{
			for(Actor* actor : actors)
			{
				std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(*actor, block);
				actor->m_hasObjectives.replaceTasks(std::move(objective));
			}
		});
		// Station.
		auto station = tgui::Button::create("station");
		m_rootGui->add(station);
		station->onClick([&]{
			for(Actor* actor : actors)
			{
				std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(*actor, block);
				actor->m_hasObjectives.replaceTasks(std::move(objective));
			}
		});
		// Kill.
		for(Actor* otherActor : block.m_hasActors.getAll())
		{
			const Faction& faction = *(*actors.begin())->getFaction();
			const Faction& otherFaction = *otherActor->getFaction();
			if(faction.enemies.contains(const_cast<Faction*>(&otherFaction)))
			{
				tgui::Button::Ptr kill = tgui::Button::create("kill");
				m_rootGui->add(kill);
				kill->onClick([&]{
					for(Actor* actor : actors)
					{
						std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(*actor, *otherActor);
						actor->m_hasObjectives.replaceTasks(std::move(objective));
					}
				});
			}
			//TODO: follow objective, attack move, targeted haul.
		}
	}
	auto items = m_window.getSelectedItems();
	if(items.size() == 1)
	{
		Item& item = **items.begin();
		if(item.m_itemType.installable)
		{
			// Install item.
			auto install = tgui::Label::create("install...");
			m_rootGui->add(install);
			install->onMouseEnter([&]{
				auto submenu = makeSubmenu(0, install);
				for(Facing facing = 0; facing < 4; ++facing)
				{
					auto button = tgui::Button::create(m_window.facingToString(facing));
					submenu->add(button);
					button->onClick([&]{ m_window.getArea()->m_hasInstallItemDesignations.at(*m_window.getFaction()).add(block, item); });
				}
			});
			// TODO: targeted haul.
		}
	}
	if(block.isSolid())
	{
		// Dig.
		auto digButton = tgui::Button::create("dig");
		m_rootGui->add(digButton);
		digButton->onClick([&]{
			for(Block& selectedBlock : m_window.getSelectedBlocks())
				if(selectedBlock.isSolid())
					m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), selectedBlock, nullptr);
			hide();
		});
		digButton->onMouseEnter([&]{
			auto subMenu = makeSubmenu(0, digButton);
			m_gameOverlayGroup->add(subMenu);
			subMenu->setPosition("rootGui.right", "digButton.top");
			for(const BlockFeatureType* blockFeatureType : BlockFeatureType::getAll())
				if(blockFeatureType->canBeHewn)
				{
					auto button = tgui::Button::create(blockFeatureType->name);
					button->onClick([&]{
						for(Block& selectedBlock : m_window.getSelectedBlocks())
							if(selectedBlock.isSolid())
								m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), selectedBlock, blockFeatureType);
						hide();
					});
				}
		});
	}
	else
	{
		// Construct.
		auto constructButton = tgui::Button::create("construct");
		m_rootGui->add(constructButton);
		constructButton->onClick([&]{
			auto subMenu = makeSubmenu(0, constructButton);
			auto materialTypeSelector = tgui::ComboBox::create();
			subMenu->add(materialTypeSelector);
			for(MaterialType& materialType : MaterialType::data)
				if(materialType.constructionData)
					materialTypeSelector->addItem(materialType.name, materialType.name);
			materialTypeSelector->onItemSelect([&](const tgui::String& name){
				const MaterialType& materialType = MaterialType::byName(name.toStdString());
				for(Block& selectedBlock : m_window.getSelectedBlocks())
					if(!selectedBlock.isSolid())
						m_window.getArea()->m_hasConstructionDesignations.designate(*m_window.getFaction(), block, nullptr, materialType);
				hide();
			});
		});
		constructButton->onMouseEnter([&]{
			auto subMenu = makeSubmenu(0, constructButton);
			m_gameOverlayGroup->add(subMenu);
			subMenu->setPosition("rootGui.right", "constructButton.top");
			for(const BlockFeatureType* blockFeatureType : BlockFeatureType::getAll())
			{
				if(blockFeatureType->canBeHewn)
				{
					auto button = tgui::Button::create(blockFeatureType->name);
					button->onClick([&]{
						auto subMenu = makeSubmenu(1, constructButton);
						auto materialTypeSelector = tgui::ComboBox::create();
						subMenu->add(materialTypeSelector);
						for(MaterialType& materialType : MaterialType::data)
							if(materialType.constructionData)
								materialTypeSelector->addItem(materialType.name, materialType.name);
						materialTypeSelector->onItemSelect([&](const tgui::String& name){
							const MaterialType& materialType = MaterialType::byName(name.toStdString());
							for(Block& selectedBlock : m_window.getSelectedBlocks())
								if(!selectedBlock.isSolid())
									m_window.getArea()->m_hasConstructionDesignations.designate(*m_window.getFaction(), selectedBlock, nullptr, materialType);
							hide();
						});
					});
				}
			}
		});
		// Farm
		if(block.m_isPartOfFarmField.contains(*m_window.getFaction()))
		{
			auto shrinkButton = tgui::Button::create("shrink farm field(s)");
			m_rootGui->add(shrinkButton);
			shrinkButton->onClick([&]{
				for(Block& block : m_window.getSelectedBlocks())
					block.m_isPartOfFarmField.remove(*m_window.getFaction());
				hide();
			});
			auto setFarmSpeciesButton = tgui::Button::create("set farm species");
			m_rootGui->add(setFarmSpeciesButton);
			setFarmSpeciesButton->onClick([&]{
				auto submenu = makeSubmenu(0, setFarmSpeciesButton);
				auto plantSpecies = tgui::ComboBox::create();
				for(const PlantSpecies& species : PlantSpecies::data)
					if(block.m_hasPlant.canGrowHereEver(species))
					{
						auto button = tgui::Button::create(species.name);
						submenu->add(button);
						button->onClick([&]{
							FarmField& field = *block.m_isPartOfFarmField.get(*m_window.getFaction());
							block.m_area->m_hasFarmFields.at(*m_window.getFaction()).setSpecies(field, species);
							hide();
						});
					}
			});
		}
		else 
		{
			auto createButton = tgui::Button::create("create farm field");	
			m_rootGui->add(createButton);
			createButton->onClick([&]{
				auto submenu = makeSubmenu(0, createButton);
				auto plantSpecies = tgui::ComboBox::create();
				for(const PlantSpecies& species : PlantSpecies::data)
					if(block.m_hasPlant.canGrowHereEver(species))
					{
						auto button = tgui::Button::create(species.name);
						submenu->add(button);
						button->onClick([&]{
							for(Block& block : m_window.getSelectedBlocks())
								block.m_isPartOfFarmField.remove(*m_window.getFaction());
							auto fieldsForFaction = block.m_area->m_hasFarmFields.at(*m_window.getFaction());
							FarmField& field = fieldsForFaction.create(m_window.getSelectedBlocks());
							fieldsForFaction.setSpecies(field, species);
							hide();
						});
					}
			});
		}
		// Actor details.
		for(Actor* actor : block.m_hasActors.getAll())
		{
			auto details = tgui::Button::create(actor->m_name);
			m_rootGui->add(details);
			details->onClick([&]{
				m_window.showActorDetail(*actor);
				hide();
			});
			details->onMouseEnter([&]{
				auto submenu = makeSubmenu(0, details);
				auto priorities = tgui::Button::create("priorities");
				submenu->add(priorities);
				priorities->onClick([&]{
					m_window.showObjectivePriority(*actor);
					hide();
				});
			});
		}
	}
}
tgui::Panel::Ptr ContextMenu::makeSubmenu(size_t index, tgui::Widget::Ptr creator)
{
	assert(m_submenus.size() + 1 >= index);
	// destroy old submenus.
	size_t i = m_submenus.size() - 1;
	while(i >= index)
		m_rootGui->remove(m_submenus[i]);
	m_submenus.resize(index);
	auto submenu = m_submenus.back();
	m_gameOverlayGroup->add(submenu);
	// If this is the first submenu then index is 0 and x position is taken from rootGui, otherwise it is taken from the previous submenu.
	auto xBinding = index ? tgui::bindRight(m_submenus[index - 1]) : tgui::bindRight(m_rootGui);
	submenu->setPosition(xBinding, creator->getPosition().y);
	return submenu;
}
void ContextMenu::hide()
{
	m_submenus.clear();
	m_rootGui->removeAllWidgets();
}
