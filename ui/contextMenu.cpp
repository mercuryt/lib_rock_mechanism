#include "contextMenu.h"
#include "blockFeature.h"
#include "config.h"
#include "plant.h"
#include "widgets.h"
#include "window.h"
#include "../engine/actor.h"
#include "../engine/goTo.h"
#include "../engine/station.h"
#include "../engine/installItem.h"
#include "../engine/kill.h"
#include "../engine/uniform.h"
#include <SFML/Graphics/Color.hpp>
#include <TGUI/Layout.hpp>
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/EditBox.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
// Menu segment
ContextMenuSegment::ContextMenuSegment(tgui::Group::Ptr overlayGroup) : m_gameOverlayGroup(overlayGroup), m_widget(tgui::Grid::create()) 
{ 
	m_widget->setOrigin(0, 0.5);
}
void ContextMenuSegment::add(tgui::Widget::Ptr widget, std::string id) 
{
	widget->setSize(width, height); 
	m_widget->add(widget, id); 
	m_widget->setWidgetCell(widget, m_count++, 1); 
}
ContextMenuSegment::~ContextMenuSegment() { m_gameOverlayGroup->remove(m_widget); }
// Menu
ContextMenu::ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup) : m_window(window), m_root(gameOverlayGroup), m_gameOverlayGroup(gameOverlayGroup)
{
	gameOverlayGroup->add(m_root.m_widget);
	m_root.m_widget->setVisible(false);
	m_root.m_widget->setOrigin(1.0, 0.5);
}
tgui::Color buttonColor{tgui::Color::Blue};
tgui::Color labelColor{tgui::Color::Black};
void ContextMenu::draw(Block& block)
{
	m_root.m_widget->setVisible(true);
	m_root.m_widget->setPosition(sf::Mouse::getPosition().x, sf::Mouse::getPosition().y);
	m_root.m_widget->removeAllWidgets();
	m_submenus.clear();
	//TODO: shift to add to end of work queue.
	if(block.isSolid())
	{
		// Dig.
		auto digButton = tgui::Button::create("dig");
		digButton->getRenderer()->setBackgroundColor(buttonColor);
		m_root.add(digButton);
		digButton->onClick([this]{
			for(Block& selectedBlock : m_window.getSelectedBlocks())
				if(selectedBlock.isSolid())
				{
					if (m_window.m_editMode)
						selectedBlock.setNotSolid();
					else
						m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), selectedBlock, nullptr);
				}
			hide();
		});
		digButton->onMouseEnter([this]{
			auto& subMenu = makeSubmenu(0);
			for(const BlockFeatureType* blockFeatureType : BlockFeatureType::getAll())
				if(blockFeatureType->canBeHewn)
				{
					auto button = tgui::Button::create(blockFeatureType->name);
					subMenu.add(button);
					button->getRenderer()->setBackgroundColor(buttonColor);
					button->onClick([this, blockFeatureType]{
						for(Block& selectedBlock : m_window.getSelectedBlocks())
							if(selectedBlock.isSolid())
							{
								if(m_window.m_editMode)
									selectedBlock.m_hasBlockFeatures.hew(*blockFeatureType);
								else
									m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), selectedBlock, blockFeatureType);
							}
						hide();
					});
				}
		});
	}
	else
	{
		if(m_window.m_editMode)
		{
			if(block.m_hasPlant.exists())
			{
				auto removePlant = tgui::Button::create("remove " + block.m_hasPlant.get().m_plantSpecies.name);
				removePlant->getRenderer()->setBackgroundColor(buttonColor);
				removePlant->onClick([this, &block]{
					block.m_hasPlant.erase();
					hide();
				});
				m_root.add(removePlant);
			}
			else if(block.getBlockBelow() && block.getBlockBelow()->isSolid())
			{
				auto addPlant = tgui::Label::create("add plant");
				addPlant->getRenderer()->setBackgroundColor(buttonColor);
				m_root.add(addPlant);
				addPlant->onMouseEnter([this, &block]{
					auto& submenu = makeSubmenu(0);
					auto speciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
					submenu.add(speciesUI);
					auto percentGrownUI = tgui::SpinControl::create();
					auto percentGrownLabel = tgui::Label::create("percent grown");
					submenu.add(percentGrownLabel);
					percentGrownLabel->getRenderer()->setBackgroundColor(labelColor);
					submenu.add(percentGrownUI);
					auto confirm = tgui::Button::create("confirm");
					confirm->getRenderer()->setBackgroundColor(buttonColor);
					submenu.add(confirm);
					confirm->onClick([&block, speciesUI, percentGrownUI]{
						const PlantSpecies& species = PlantSpecies::byName(speciesUI->getSelectedItemId().toStdString());
						block.m_hasPlant.createPlant(species, percentGrownUI->getValue());
					});
				});
			}
		}
		auto& actors = m_window.getSelectedActors();
		if(actors.size())
		{
			// Go.
			auto goTo = tgui::Button::create("go to");
			goTo->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(goTo);
			goTo->onClick([this, &block]{
				for(Actor* actor : m_window.getSelectedActors())
				{
					std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(*actor, block);
					actor->m_hasObjectives.replaceTasks(std::move(objective));
					hide();
				}
			});
			// Station.
			auto station = tgui::Button::create("station");
			station->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(station);
			station->onClick([this, &block]{
				for(Actor* actor : m_window.getSelectedActors())
				{
					std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(*actor, block);
					actor->m_hasObjectives.replaceTasks(std::move(objective));
				}
				hide();
			});
			// Kill.
			for(Actor* otherActor : block.m_hasActors.getAll())
			{
				const Faction& faction = *(*actors.begin())->getFaction();
				const Faction& otherFaction = *otherActor->getFaction();
				if(faction.enemies.contains(const_cast<Faction*>(&otherFaction)))
				{
					tgui::Button::Ptr kill = tgui::Button::create("kill");
					kill->getRenderer()->setBackgroundColor(buttonColor);
					m_root.add(kill);
					kill->onClick([this, otherActor]{
						for(Actor* actor : m_window.getSelectedActors())
						{
							std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(*actor, *otherActor);
							actor->m_hasObjectives.replaceTasks(std::move(objective));
						}
						hide();
					});
				}
				//TODO: follow objective, attack move, targeted haul.
			}
			if(actors.size() == 1)
			{
				Actor& actor = **actors.begin();
				// Equip.
				for(Item* item : block.m_hasItems.getAll())
					if(actor.m_equipmentSet.canEquipCurrently(*item))
					{
						ItemQuery query(*item);
						auto button = tgui::Button::create(m_window.displayNameForItem(*item));
						button->getRenderer()->setBackgroundColor(buttonColor);
						m_root.add(button);
						button->onClick([this, &actor, query] () mutable {
							std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(actor, query);
							actor.m_hasObjectives.replaceTasks(std::move(objective));
							hide();
						});
					}
			}
		}
		// Items.
		auto items = m_window.getSelectedItems();
		if(items.size() == 1)
		{
			Item& item = **items.begin();
			if(item.m_itemType.installable)
			{
				// Install item.
				auto install = tgui::Label::create("install...");
				install->getRenderer()->setBackgroundColor(buttonColor);
				m_root.add(install);
				install->onMouseEnter([this, &block, &item]{
					auto& submenu = makeSubmenu(0);
					for(Facing facing = 0; facing < 4; ++facing)
					{
						auto button = tgui::Button::create(m_window.facingToString(facing));
						button->getRenderer()->setBackgroundColor(buttonColor);
						submenu.add(button);
						button->onClick([this, &block, &item, facing]{ 
							m_window.getArea()->m_hasInstallItemDesignations.at(*m_window.getFaction()).add(block, item, facing, *m_window.getFaction()); 
							hide(); 
						});
					}
				});
				// TODO: targeted haul.
			}
		}
		for(Item* item : block.m_hasItems.getAll())
		{
			auto name = m_window.displayNameForItem(*item);
			auto button = tgui::Button::create(name + L" install at...");
			button->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(button);
			button->onClick([this, item]{ m_window.setItemToInstall(*item); hide(); });
			if(m_window.m_editMode)
			{
				auto destroy = tgui::Button::create(name + L" destroy");
				destroy->onClick([item]{ item->destroy(); });
				m_root.add(destroy);
			}
		}
		// Construct.
		auto constructLabel = tgui::Label::create("construct");
		constructLabel->getRenderer()->setBackgroundColor(buttonColor);
		m_root.add(constructLabel);
		constructLabel->onMouseEnter([this, &block]{
			auto& subMenu = makeSubmenu(0);
			auto materialTypeSelector = widgetUtil::makeMaterialSelectUI();
			subMenu.add(materialTypeSelector);
			static bool constructed = false;
			if(m_window.m_editMode)
			{
				auto constructedUI = tgui::CheckBox::create();
				subMenu.add(constructedUI);
				constructedUI->setChecked(constructed);
				constructedUI->onChange([&](bool value){ constructed = value; });
			}
			std::map<std::string, const BlockFeatureType*> m_buttons{{"solid", nullptr}};
			for(const BlockFeatureType* blockFeatureType : BlockFeatureType::getAll())
				m_buttons[blockFeatureType->name] = blockFeatureType;
			for(auto& [name, type] : m_buttons)
			{
				auto button = tgui::Button::create(name);
				subMenu.add(button);
				button->onClick([this, &block, type, materialTypeSelector]{
					const MaterialType& materialType = MaterialType::byName(materialTypeSelector->getSelectedItemId().toStdString());
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block& selectedBlock : m_window.getSelectedBlocks())
						if(!selectedBlock.isSolid())
						{
							if (m_window.m_editMode)
							{
								if(type == nullptr)
									selectedBlock.setSolid(materialType, constructed);
								else
									if(!constructed)
									{
										if(!block.isSolid())
											block.setSolid(materialType);
										selectedBlock.m_hasBlockFeatures.hew(*type);
									}
									else
										selectedBlock.m_hasBlockFeatures.construct(*type, materialType);

							}
							else
								m_window.getArea()->m_hasConstructionDesignations.designate(*m_window.getFaction(), selectedBlock, type, materialType);
						}
					hide();
				});
			}
		});
		// Farm
		if(block.m_isPartOfFarmField.contains(*m_window.getFaction()))
		{
			auto shrinkButton = tgui::Button::create("shrink farm field(s)");
			shrinkButton->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(shrinkButton);
			shrinkButton->onClick([this]{
				for(Block& block : m_window.getSelectedBlocks())
					block.m_isPartOfFarmField.remove(*m_window.getFaction());
				hide();
			});
			auto setFarmSpeciesButton = tgui::Button::create("set farm species");
			setFarmSpeciesButton->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(setFarmSpeciesButton);
			setFarmSpeciesButton->onClick([this, &block]{
				auto& submenu = makeSubmenu(0);
				auto plantSpeciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
				submenu.add(plantSpeciesUI);
				plantSpeciesUI->onItemSelect([this, &block](const tgui::String& string)
				{
					const PlantSpecies& species = PlantSpecies::byName(string.toStdString());
					FarmField& field = *block.m_isPartOfFarmField.get(*m_window.getFaction());
					block.m_area->m_hasFarmFields.at(*m_window.getFaction()).setSpecies(field, species);
					hide();
					
				});
			});
		}
		else if(block.getBlockBelow() && block.getBlockBelow()->m_hasPlant.anythingCanGrowHereEver())
		{
			auto createLabel = tgui::Label::create("create farm field");	
			createLabel->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(createLabel);
			createLabel->onMouseEnter([this, &block]{
				auto& submenu = makeSubmenu(0);
				auto plantSpeciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
				submenu.add(plantSpeciesUI);
				plantSpeciesUI->onItemSelect([this, &block](const tgui::String name){ 
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block& block : m_window.getSelectedBlocks())
						block.m_isPartOfFarmField.remove(*m_window.getFaction());
					auto fieldsForFaction = block.m_area->m_hasFarmFields.at(*m_window.getFaction());
					FarmField& field = fieldsForFaction.create(m_window.getSelectedBlocks());
					fieldsForFaction.setSpecies(field, PlantSpecies::byName(name.toStdString()));
					hide();
				});
			});
		}
		// Actor details.
		for(Actor* actor : block.m_hasActors.getAll())
		{
			auto details = tgui::Button::create(actor->m_name);
			details->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(details);
			details->onClick([this, actor]{
				m_window.showActorDetail(*actor);
				hide();
			});
			details->onMouseEnter([this, actor]{
				auto& submenu = makeSubmenu(0);
				auto priorities = tgui::Button::create("priorities");
				priorities->getRenderer()->setBackgroundColor(buttonColor);
				submenu.add(priorities);
				priorities->onClick([this, actor]{
					m_window.showObjectivePriority(*actor);
					hide();
				});
				if(m_window.m_editMode)
				{
					auto destroy = tgui::Button::create("destroy");
					destroy->getRenderer()->setBackgroundColor(buttonColor);
					destroy->onClick([this, actor]{ m_window.getSimulation()->destroyActor(*actor);});
					submenu.add(destroy);
				}
			});
		}
		if(m_window.m_editMode)
		{
			auto createActor = tgui::Label::create("create actor");
			createActor->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(createActor);
			createActor->onMouseEnter([this, &block]{
				auto& submenu = makeSubmenu(0);
				auto nameLabel = tgui::Label::create("name");
				nameLabel->getRenderer()->setBackgroundColor(labelColor);
				submenu.add(nameLabel);
				auto name = tgui::EditBox::create();
				submenu.add(name);
				auto speciesLabel = tgui::Label::create("species");
				speciesLabel->getRenderer()->setBackgroundColor(labelColor);
				submenu.add(speciesLabel);
				auto speciesUI = widgetUtil::makeAnimalSpeciesSelectUI();
				submenu.add(speciesUI);
				//TODO: generate a default name when species is selected if name is blank.
				auto confirm = tgui::Button::create("create");
				confirm->getRenderer()->setBackgroundColor(buttonColor);
				submenu.add(confirm);
				confirm->onClick([this, &block, speciesUI]{
					m_window.getSimulation()->createActorWithRandomAge(AnimalSpecies::byName(speciesUI->getSelectedItemId().toStdString()), block);
					hide();
				});
				
			});

			auto createFluidSource = tgui::Label::create("create fluid");
			createFluidSource->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(createFluidSource);
			static Volume fluidLevel = Config::maxBlockVolume;
			createFluidSource->onMouseEnter([this, &block]{
				auto& submenu = makeSubmenu(0);
				auto fluidTypeLabel = tgui::Label::create("fluid type");
				submenu.add(fluidTypeLabel);
				fluidTypeLabel->getRenderer()->setBackgroundColor(labelColor);
				auto fluidTypeUI = widgetUtil::makeFluidTypeSelectUI();
				submenu.add(fluidTypeUI);
				auto levelLabel = tgui::Label::create("level");
				levelLabel->getRenderer()->setBackgroundColor(labelColor);
				submenu.add(levelLabel);
				auto levelUI = tgui::SpinControl::create();
				levelUI->setMinimum(1);
				levelUI->onValueChange([&](const float value){fluidLevel = value;});
				levelUI->setValue(fluidLevel);
				submenu.add(levelUI);
				auto createFluid = tgui::Button::create("create fluid");
				createFluid->getRenderer()->setBackgroundColor(buttonColor);
				submenu.add(createFluid);
				createFluid->onClick([this, &block, fluidTypeUI, levelUI]{
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block& block : m_window.getSelectedBlocks())
						block.addFluid(fluidLevel, FluidType::byName(fluidTypeUI->getSelectedItemId().toStdString()));
					hide();
				});
				if(!m_window.getArea()->m_fluidSources.contains(block))
				{
					auto createSource = tgui::Button::create("create source");
					createSource->getRenderer()->setBackgroundColor(buttonColor);
					submenu.add(createSource);
					createSource->onClick([this, &block, fluidTypeUI, levelUI]{
						if(m_window.getSelectedBlocks().empty())
							m_window.selectBlock(block);
						for(Block& selectedBlock: m_window.getSelectedBlocks())
							m_window.getArea()->m_fluidSources.create(
								selectedBlock, 
								FluidType::byName(fluidTypeUI->getSelectedItemId().toStdString()), 
								fluidLevel
							);
						hide();
					});
				}
			});
		}
		if(m_window.getArea()->m_fluidSources.contains(block))
		{
			auto removeFluidSourceButton = tgui::Label::create("remove fluid source");
			removeFluidSourceButton->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(removeFluidSourceButton);
			removeFluidSourceButton->onClick([&]{
				m_window.getArea()->m_fluidSources.destroy(block);
				hide();
			});
		}
	}
}
ContextMenuSegment& ContextMenu::makeSubmenu(size_t index)
{
	assert(m_submenus.size() + 1 > index);
	// destroy old submenus.
	m_submenus.resize(index, {m_gameOverlayGroup});
	m_submenus.emplace_back(m_gameOverlayGroup);
	auto& submenu = m_submenus.back();
	m_gameOverlayGroup->add(submenu.m_widget);
	ContextMenuSegment& previous = index == 0 ? m_root : m_submenus[index - 1];
	sf::Vector2i pixelPos = sf::Mouse::getPosition();
	submenu.m_widget->setPosition(tgui::bindRight(previous.m_widget), pixelPos.y);
	return submenu;
}
void ContextMenu::hide()
{
	m_submenus.clear();
	m_root.m_widget->removeAllWidgets();
}
