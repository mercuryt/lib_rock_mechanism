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
#include <TGUI/Widgets/HorizontalLayout.hpp>
// Menu segment
ContextMenuSegment::ContextMenuSegment(tgui::Group::Ptr overlayGroup) : 
	m_gameOverlayGroup(overlayGroup), m_panel(tgui::Panel::create()), m_grid(tgui::Grid::create()) 
{ 
	m_panel->add(m_grid);
	m_panel->setSize(tgui::bindWidth(m_grid) + 4, tgui::bindHeight(m_grid) + 4);
	m_grid->setPosition(2,2);
}
void ContextMenuSegment::add(tgui::Widget::Ptr widget, std::string id) 
{
	widget->setSize(width, height); 
	m_grid->add(widget, id); 
	m_grid->setWidgetCell(widget, m_count++, 1, tgui::Grid::Alignment::Center, tgui::Padding{1, 5});
}
ContextMenuSegment::~ContextMenuSegment() { m_gameOverlayGroup->remove(m_panel); }
// Menu
ContextMenu::ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup) : 
	m_window(window), m_root(gameOverlayGroup), m_gameOverlayGroup(gameOverlayGroup)
{
	gameOverlayGroup->add(m_root.m_panel);
	m_root.m_panel->setVisible(false);
}
tgui::Color buttonColor{tgui::Color::Blue};
tgui::Color labelColor{tgui::Color::Black};
void ContextMenu::draw(Block& block)
{
	m_root.m_panel->setVisible(true);
	m_root.m_panel->setOrigin(1, getOriginYForMousePosition());
	auto xPosition = std::max(ContextMenuSegment::width, sf::Mouse::getPosition().x);
	m_root.m_panel->setPosition(xPosition, sf::Mouse::getPosition().y);
	m_root.m_grid->removeAllWidgets();
	m_submenus.clear();
	auto blockInfoButton = tgui::Button::create("location info");
	m_root.add(blockInfoButton);
	blockInfoButton->onClick([&]{ m_window.getGameOverlay().drawInfoPopup(block); });
	//TODO: shift to add to end of work queue.
	if(block.isSolid())
	{
		// Dig.
		auto digButton = tgui::Button::create("dig");
		digButton->getRenderer()->setBackgroundColor(buttonColor);
		m_root.add(digButton);
		digButton->onClick([this]{
			for(Block* selectedBlock : m_window.getSelectedBlocks())
				if(selectedBlock->isSolid())
				{
					if (m_window.m_editMode)
						selectedBlock->setNotSolid();
					else
						m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), *selectedBlock, nullptr);
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
						for(Block* selectedBlock : m_window.getSelectedBlocks())
							if(selectedBlock->isSolid())
							{
								if(m_window.m_editMode)
									selectedBlock->m_hasBlockFeatures.hew(*blockFeatureType);
								else
									m_window.getArea()->m_hasDigDesignations.designate(*m_window.getFaction(), *selectedBlock, blockFeatureType);
							}
						hide();
					});
				}
		});
	}
	else
	{
		// Actor submenu.
		for(Actor* actor : block.m_hasActors.getAll())
		{
			auto label = tgui::Label::create(actor->m_name);
			label->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(label);
			label->onMouseEnter([this, actor]{
				auto& submenu = makeSubmenu(0);
				auto actorInfoButton = tgui::Button::create("info");
				submenu.add(actorInfoButton);
				actorInfoButton->onClick([this, actor]{ m_window.getGameOverlay().drawInfoPopup(*actor);});
				auto actorDetailButton = tgui::Button::create("details");
				submenu.add(actorDetailButton);
				actorDetailButton->onClick([this, actor]{ m_window.showActorDetail(*actor); hide(); });
				if(m_window.m_editMode)
				{
					auto actorEditButton = tgui::Button::create("edit");
					submenu.add(actorEditButton);
					actorEditButton->onClick([this, actor]{ m_window.showEditActor(*actor); hide(); });
				}
				if(m_window.m_editMode || (m_window.getFaction() && m_window.getFaction() == actor->getFaction()))
				{
					auto priorities = tgui::Button::create("priorities");
					priorities->getRenderer()->setBackgroundColor(buttonColor);
					submenu.add(priorities);
					priorities->onClick([this, actor]{
						m_window.showObjectivePriority(*actor);
						hide();
					});
				}
				if(m_window.m_editMode)
				{
					auto destroy = tgui::Button::create("destroy");
					destroy->getRenderer()->setBackgroundColor(buttonColor);
					destroy->onClick([this, actor]{ m_window.getSimulation()->destroyActor(*actor);});
					submenu.add(destroy);
				}
				auto& actors = m_window.getSelectedActors();
				if(!actors.empty() && (m_window.m_editMode || m_window.getFaction() != actor->getFaction()))
				{
					tgui::Button::Ptr kill = tgui::Button::create("kill");
					submenu.add(kill);
					kill->onClick([this, actor, &actors]{
						for(Actor* selectedActor : actors)
						{
							std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(*selectedActor, *actor);
							selectedActor->m_hasObjectives.replaceTasks(std::move(objective));
						}
						hide();
					});
				}
			});
		}
		// Plant submenu.
		if(block.m_hasPlant.exists())
		{
			auto label = tgui::Label::create(block.m_hasPlant.get().m_plantSpecies.name);
			label->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(label);
			label->onMouseEnter([this, &block]{
				auto& submenu = makeSubmenu(0);
				auto infoButton = tgui::Button::create("info");
				submenu.add(infoButton);
				infoButton->onClick([this, &block]{ m_window.getGameOverlay().drawInfoPopup(block);});
				auto cutDown = tgui::Button::create("cut down");
				submenu.add(cutDown);
				cutDown->onClick([this, &block]{ 
					const PlantSpecies& species = block.m_hasPlant.get().m_plantSpecies;
					//TODO: cut down non trees.
					if(!species.isTree)
						return;
					for(Block* selectedBlock : m_window.getSelectedBlocks())
						if(selectedBlock->m_hasPlant.exists() && selectedBlock->m_hasPlant.get().m_plantSpecies == species)
							m_window.getArea()->m_hasWoodCuttingDesignations.designate(*m_window.getFaction(), *selectedBlock); 
				});
				if(m_window.m_editMode)
				{
					auto removePlant = tgui::Button::create("remove " + block.m_hasPlant.get().m_plantSpecies.name);
					removePlant->onClick([this, &block]{
						const PlantSpecies& species = block.m_hasPlant.get().m_plantSpecies;
						for(Block* selectedBlock : m_window.getSelectedBlocks())
							if(selectedBlock->m_hasPlant.exists() && selectedBlock->m_hasPlant.get().m_plantSpecies == species)
								selectedBlock->m_hasPlant.get().remove();
						for(Plant* plant : m_window.getSelectedPlants())
							if(plant->m_plantSpecies == species)
								plant->remove();
						hide();
					});
					submenu.add(removePlant);
				}
			});
		}
		// Item submenu.
		if(!block.m_hasItems.empty())
			for(Item* item : block.m_hasItems.getAll())
			{
				auto label = tgui::Label::create(m_window.displayNameForItem(*item));
				m_root.add(label);
				label->onMouseEnter([this, item]{
					auto& submenu = makeSubmenu(0);
					auto info = tgui::Button::create("info");
					submenu.add(info);
					info->onClick([this, item]{ m_window.getGameOverlay().drawInfoPopup(*item); });
					if(m_window.getFaction())
					{
						bool marked = item->m_canBeStockPiled.contains(*m_window.getFaction());
						auto stockpileUI = tgui::Button::create(marked ? "do not stockpile" : "stockpile");
						stockpileUI->onClick([item, marked, this]{ 
							if(marked) 
								item->m_canBeStockPiled.unset(*m_window.getFaction());
							else
								item->m_canBeStockPiled.set(*m_window.getFaction());
							hide();
						});
					}
					if(!m_window.getSelectedActors().empty())
					{
						//TODO: targeted haul, installation , stockpile
					}
					if(m_window.m_editMode)
					{
						auto destroy = tgui::Button::create("destroy");
						submenu.add(destroy);
						destroy->onClick([item]{ item->destroy(); });
					}
					auto& actors = m_window.getSelectedActors();
					if(actors.size() == 1)
					{
						Actor& actor = **actors.begin();
						// Equip.
						if(actor.m_equipmentSet.canEquipCurrently(*item))
						{
							ItemQuery query(*item);
							auto button = tgui::Button::create(m_window.displayNameForItem(*item));
							button->getRenderer()->setBackgroundColor(buttonColor);
							submenu.add(button);
							button->onClick([this, &actor, query] () mutable {
								std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(actor, query);
								actor.m_hasObjectives.replaceTasks(std::move(objective));
								hide();
							});
						}
					}
					if(m_window.getFaction())
					{
						auto installAt = tgui::Button::create("install at...");
						installAt->getRenderer()->setBackgroundColor(buttonColor);
						submenu.add(installAt);
						installAt->onClick([this, item]{ m_window.setItemToInstall(*item); hide(); });
					}
				});
			}
		if(m_window.m_editMode)
		{
			// Fluids.
			for(auto& [fluidType, pair] : block.m_fluids)
			{
				auto volume = pair.first;
				auto button = tgui::Button::create("remove " + std::to_string(volume) + " " + fluidType->name);
				m_root.add(button);
				button->onClick([&]{ 
					for(auto& selectedBlock : m_window.getSelectedBlocks())
					{
						auto contains = selectedBlock->volumeOfFluidTypeContains(*fluidType);
						if(!contains) return;
						selectedBlock->removeFluid(contains, *fluidType);
					}
					hide();
				});
			}
			// Plant.
			if(block.m_hasPlant.anythingCanGrowHereEver())
			{
				auto addPlant = tgui::Label::create("add plant");
				addPlant->getRenderer()->setBackgroundColor(buttonColor);
				m_root.add(addPlant);
				static Percent percentGrown = 100;
				addPlant->onMouseEnter([this, &block]{
					auto& submenu = makeSubmenu(0);
					auto speciesUI = widgetUtil::makePlantSpeciesSelectUI(&block);
					submenu.add(speciesUI);
					auto percentGrownUI = tgui::SpinControl::create();
					auto percentGrownLabel = tgui::Label::create("percent grown");
					submenu.add(percentGrownLabel);
					percentGrownLabel->getRenderer()->setBackgroundColor(labelColor);
					submenu.add(percentGrownUI);
					percentGrownUI->setMinimum(1);
					percentGrownUI->setMaximum(100);
					percentGrownUI->setValue(percentGrown);
					percentGrownUI->onValueChange([](const float value){ percentGrown = value; });
					auto confirm = tgui::Button::create("confirm");
					confirm->getRenderer()->setBackgroundColor(buttonColor);
					submenu.add(confirm);
					confirm->onClick([this, speciesUI, percentGrownUI]{
						if(!speciesUI->getSelectedItem().empty())
						{
							const PlantSpecies& species = PlantSpecies::byName(speciesUI->getSelectedItemId().toStdString());
							for(Block* selectedBlock : m_window.getSelectedBlocks())
								if(!selectedBlock->m_hasPlant.exists() && !selectedBlock->isSolid() && selectedBlock->m_hasPlant.canGrowHereEver(species))
									selectedBlock->m_hasPlant.createPlant(species, percentGrownUI->getValue());
						}
						hide();
					});
				});
			}
			// Item.
			auto addItem = tgui::Label::create("add item");
			addItem->getRenderer()->setBackgroundColor(buttonColor);
			m_root.add(addItem);
			static uint32_t quantity = 1;
			static uint32_t quality = 0;
			static uint32_t percentWear = 0;
			addItem->onMouseEnter([this, &block]{
				auto& submenu = makeSubmenu(0);
				auto itemTypeSelectUI = widgetUtil::makeItemTypeSelectUI();
				submenu.add(itemTypeSelectUI);
				auto materialTypeSelectUI = widgetUtil::makeMaterialSelectUI();
				submenu.add(materialTypeSelectUI);
				std::function<void(const ItemType&)> onSelect = [this, itemTypeSelectUI, materialTypeSelectUI, &block](const ItemType& itemType){
					if(itemType.generic)
					{
						auto& subSubMenu = makeSubmenu(1);
						auto quantityLabel = tgui::Label::create("quantity");
						subSubMenu.add(quantityLabel);
						quantityLabel->getRenderer()->setBackgroundColor(labelColor);
						auto quantityUI = tgui::SpinControl::create();
						quantityUI->setMinimum(1);
						quantityUI->setMaximum(INT16_MAX);
						quantityUI->setValue(quantity);
						quantityUI->onValueChange([](const float value){ quantity = value; });
						subSubMenu.add(quantityUI);
						auto confirm = tgui::Button::create("confirm");
						confirm->getRenderer()->setBackgroundColor(buttonColor);
						subSubMenu.add(confirm);
						confirm->onClick([this, itemTypeSelectUI, materialTypeSelectUI]{
							const MaterialType& materialType = MaterialType::byName(materialTypeSelectUI->getSelectedItemId().toStdString());
							const ItemType& itemType = ItemType::byName(itemTypeSelectUI->getSelectedItemId().toStdString());
							for(Block* selectedBlock : m_window.getSelectedBlocks())
							{
								Item& item = m_window.getSimulation()->createItemGeneric(itemType, materialType, quantity);
								item.setLocation(*selectedBlock);
							}
							hide();
						});
					}
					else
					{
						auto& subSubMenu = makeSubmenu(1);
						auto qualityLabel = tgui::Label::create("quality");
						subSubMenu.add(qualityLabel);
						qualityLabel->getRenderer()->setBackgroundColor(labelColor);
						auto qualityUI = tgui::SpinControl::create();
						qualityUI->setMinimum(1);
						qualityUI->setMaximum(100);
						qualityUI->setValue(quality);
						qualityUI->onValueChange([](const float value){ quality = value; });
						subSubMenu.add(qualityUI);
						auto percentWearLabel = tgui::Label::create("percentWear");
						subSubMenu.add(percentWearLabel);
						percentWearLabel->getRenderer()->setBackgroundColor(labelColor);
						auto percentWearUI = tgui::SpinControl::create();
						percentWearUI->setMinimum(1);
						percentWearUI->setMaximum(100);
						percentWearUI->setValue(percentWear);
						percentWearUI->onValueChange([](const float value){ percentWear = value; });
						subSubMenu.add(percentWearUI);
						auto confirm = tgui::Button::create("confirm");
						confirm->getRenderer()->setBackgroundColor(buttonColor);
						subSubMenu.add(confirm);
						confirm->onClick([this, itemTypeSelectUI, materialTypeSelectUI]{
							const MaterialType& materialType = MaterialType::byName(materialTypeSelectUI->getSelectedItemId().toStdString());
							const ItemType& itemType = ItemType::byName(itemTypeSelectUI->getSelectedItemId().toStdString());
							for(Block* selectedBlock : m_window.getSelectedBlocks())
							{
								Item& item = m_window.getSimulation()->createItemNongeneric(itemType, materialType, quality, percentWear);
								item.setLocation(*selectedBlock);
							}
							hide();
						});
					}
				};
				itemTypeSelectUI->onItemSelect([onSelect](const tgui::String& value){
					const ItemType& itemType = ItemType::byName(value.toStdString());
					onSelect(itemType);
				});
				const ItemType& itemType = ItemType::byName(itemTypeSelectUI->getSelectedItemId().toStdString());
				onSelect(itemType);
			});
		}
		// Orders for selected actors.
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
		if(block.m_hasPlant.exists())
		{
			const Plant& plant = block.m_hasPlant.get();
			auto label = tgui::Label::create(plant.m_plantSpecies.name);
			m_root.add(label);
			label->onMouseEnter([this, &plant]{
				auto& submenu = makeSubmenu(0);
				auto info = tgui::Button::create("info");
				submenu.add(info);
				info->onClick([this, &plant]{ m_window.getGameOverlay().drawInfoPopup(plant); });
			});
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
				auto label = tgui::Label::create("constructed");
				subMenu.add(label);
				auto constructedUI = tgui::CheckBox::create();
				subMenu.add(constructedUI);
				constructedUI->setSize(10, 10);
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
				button->onClick([this, &block, type, materialTypeSelector, button]{
					const MaterialType& materialType = MaterialType::byName(materialTypeSelector->getSelectedItemId().toStdString());
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block* selectedBlock : m_window.getSelectedBlocks())
						if(!selectedBlock->isSolid())
						{
							if (m_window.m_editMode)
							{
								if(type == nullptr)
									selectedBlock->setSolid(materialType, constructed);
								else
									if(!constructed)
									{
										if(!block.isSolid())
											block.setSolid(materialType);
										selectedBlock->m_hasBlockFeatures.hew(*type);
									}
									else
										selectedBlock->m_hasBlockFeatures.construct(*type, materialType);

							}
							else
								m_window.getArea()->m_hasConstructionDesignations.designate(*m_window.getFaction(), *selectedBlock, type, materialType);
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
				for(Block* selectedBlock : m_window.getSelectedBlocks())
					selectedBlock->m_isPartOfFarmField.remove(*m_window.getFaction());
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
					for(Block* selectedBlock : m_window.getSelectedBlocks())
						selectedBlock->m_isPartOfFarmField.remove(*m_window.getFaction());
					auto fieldsForFaction = block.m_area->m_hasFarmFields.at(*m_window.getFaction());
					FarmField& field = fieldsForFaction.create(m_window.getSelectedBlocks());
					fieldsForFaction.setSpecies(field, PlantSpecies::byName(name.toStdString()));
					hide();
				});
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
				levelUI->setMaximum(Config::maxBlockVolumeHardLimit);
				levelUI->onValueChange([&](const float value){fluidLevel = value;});
				levelUI->setValue(fluidLevel);
				submenu.add(levelUI);
				auto createFluid = tgui::Button::create("create fluid");
				createFluid->getRenderer()->setBackgroundColor(buttonColor);
				submenu.add(createFluid);
				createFluid->onClick([this, &block, fluidTypeUI, levelUI]{
					if(m_window.getSelectedBlocks().empty())
						m_window.selectBlock(block);
					for(Block* selectedBlock : m_window.getSelectedBlocks())
						if(!selectedBlock->isSolid())
							selectedBlock->addFluid(fluidLevel, FluidType::byName(fluidTypeUI->getSelectedItemId().toStdString()));
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
						for(Block* selectedBlock: m_window.getSelectedBlocks())
							m_window.getArea()->m_fluidSources.create(
								*selectedBlock, 
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
	m_gameOverlayGroup->add(submenu.m_panel);
	ContextMenuSegment& previous = index == 0 ? m_root : m_submenus.at(index - 1);
	sf::Vector2i pixelPos = sf::Mouse::getPosition();
	submenu.m_panel->setOrigin(0, getOriginYForMousePosition());
	submenu.m_panel->setPosition(tgui::bindRight(previous.m_panel), pixelPos.y);
	return submenu;
}
void ContextMenu::hide()
{
	m_window.getGameOverlay().unfocusUI();
	m_submenus.clear();
	m_root.m_grid->removeAllWidgets();
	m_root.m_panel->setVisible(false);
}
[[nodiscard]] float ContextMenu::getOriginYForMousePosition() const
{
	sf::Vector2i pixelPos = sf::Mouse::getPosition();
	int windowHeight = m_window.getRenderWindow().getSize().y;
	if(pixelPos.y < ((int)m_window.getRenderWindow().getSize().y / 4))
		return 0;
	else if(pixelPos.y > (windowHeight * 3) / 4)
		return 1;
	return 0.5;
}
