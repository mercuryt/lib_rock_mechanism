#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../uiUtil.h"
#include "../../engine/block.h"
#include "../../engine/objectives/equipItem.h"
#include "../../engine/objectives/unequipItem.h"
void ContextMenu::drawItemControls(Block& block)
{
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
					bool itemAtClickedLocationMarked = item->m_canBeStockPiled.contains(*m_window.getFaction());
					auto stockpileUI = tgui::Button::create(itemAtClickedLocationMarked ? "do not stockpile" : "stockpile");
					submenu.add(stockpileUI);
					stockpileUI->onClick([item, itemAtClickedLocationMarked, this]{ 
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						if(m_window.getSelectedItems().empty())
							m_window.selectItem(*item);
						for(Item* selectedItem : m_window.getSelectedItems())
						{
							bool selectedItemMarked = selectedItem->m_canBeStockPiled.contains(*m_window.getFaction());
							if(!itemAtClickedLocationMarked) 
							{
								if(!selectedItemMarked)
									m_window.getArea()->m_hasStockPiles.at(*m_window.getFaction()).addItem(*selectedItem);
							}
							else if(selectedItemMarked)
								m_window.getArea()->m_hasStockPiles.at(*m_window.getFaction()).removeItem(*selectedItem);
						}
						hide();
					});
				}
				//TODO: targeted haul, installation , stockpile
				if(m_window.m_editMode)
				{
					auto destroy = tgui::Button::create("destroy");
					submenu.add(destroy);
					destroy->onClick([this, item]{ 
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						m_window.deselectAll();
						item->destroy(); 
						hide();
					});
				}
				auto& actors = m_window.getSelectedActors();
				if(actors.size() == 1)
				{
					Actor& actor = **actors.begin();
					// Equip.
					if(actor.m_equipmentSet.canEquipCurrently(*item))
					{
						auto button = tgui::Button::create("equip");
						button->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
						submenu.add(button);
						button->onClick([this, &actor, item] () mutable {
							std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
							std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(actor, *item);
							actor.m_hasObjectives.replaceTasks(std::move(objective));
							hide();
						});
					}
				}
				if(m_window.getFaction())
				{
					auto installAt = tgui::Button::create("install at...");
					installAt->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
					submenu.add(installAt);
					installAt->onClick([this, item]{ 
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						m_window.setItemToInstall(*item); 
						hide(); 
					});
				}
			});
		}
	auto& actors = m_window.getSelectedActors();
	if(actors.size() == 1)
	{
		Actor& actor = **actors.begin();
		if(!actor.m_equipmentSet.empty())
		{
			auto unequip = tgui::Label::create("unequip");
			m_root.add(unequip);
			unequip->onMouseEnter([this, &block, &actor]{
				auto& submenu = makeSubmenu(0);
				for(Item* item : actor.m_equipmentSet.getAll())
				{
					auto button = tgui::Label::create(UIUtil::describeItem(*item));
					submenu.add(button);
					button->onClick([this, &block, &actor, item]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						std::unique_ptr<Objective> objective = std::make_unique<UnequipItemObjective>(actor, *item, block);
						actor.m_hasObjectives.replaceTasks(std::move(objective));
						hide();
					});
				}
			});
		}
	}
	if(m_window.m_editMode)
	{
		auto addItem = tgui::Label::create("add item");
		addItem->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
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
					quantityLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					auto quantityUI = tgui::SpinControl::create();
					quantityUI->setMinimum(1);
					quantityUI->setMaximum(INT16_MAX);
					quantityUI->setValue(quantity);
					quantityUI->onValueChange([](const float value){ quantity = value; });
					subSubMenu.add(quantityUI);
					auto confirm = tgui::Button::create("confirm");
					confirm->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
					subSubMenu.add(confirm);
					confirm->onClick([this, itemTypeSelectUI, materialTypeSelectUI, &block]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						const MaterialType& materialType = MaterialType::byName(materialTypeSelectUI->getSelectedItemId().toStdString());
						const ItemType& itemType = ItemType::byName(itemTypeSelectUI->getSelectedItemId().toStdString());
						if(m_window.getSelectedBlocks().empty())
							m_window.selectBlock(block);
						for(Block* selectedBlock : m_window.getSelectedBlocks())
						{
							static const MoveType& none = MoveType::byName("none");
							if(!selectedBlock->m_hasShapes.shapeAndMoveTypeCanEnterEverWithAnyFacing(itemType.shape, none))
								continue;
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
					qualityLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					auto qualityUI = tgui::SpinControl::create();
					qualityUI->setMinimum(1);
					qualityUI->setMaximum(100);
					qualityUI->setValue(quality);
					qualityUI->onValueChange([](const float value){ quality = value; });
					subSubMenu.add(qualityUI);
					auto percentWearLabel = tgui::Label::create("percentWear");
					subSubMenu.add(percentWearLabel);
					percentWearLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					auto percentWearUI = tgui::SpinControl::create();
					percentWearUI->setMinimum(1);
					percentWearUI->setMaximum(100);
					percentWearUI->setValue(percentWear);
					percentWearUI->onValueChange([](const float value){ percentWear = value; });
					subSubMenu.add(percentWearUI);
					auto confirm = tgui::Button::create("confirm");
					confirm->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
					subSubMenu.add(confirm);
					confirm->onClick([this, itemTypeSelectUI, materialTypeSelectUI, &block]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						const MaterialType& materialType = MaterialType::byName(materialTypeSelectUI->getSelectedItemId().toStdString());
						const ItemType& itemType = ItemType::byName(itemTypeSelectUI->getSelectedItemId().toStdString());
						if(m_window.getSelectedBlocks().empty())
							m_window.selectBlock(block);
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
	// Items.
	auto items = m_window.getSelectedItems();
	if(items.size() == 1)
	{
		Item& item = **items.begin();
		if(item.m_itemType.installable)
		{
			// Install item.
			auto install = tgui::Label::create("install...");
			install->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			m_root.add(install);
			install->onMouseEnter([this, &block, &item]{
				auto& submenu = makeSubmenu(0);
				for(Facing facing = 0; facing < 4; ++facing)
				{
					auto button = tgui::Button::create(m_window.facingToString(facing));
					button->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
					submenu.add(button);
					button->onClick([this, &block, &item, facing]{ 
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						m_window.getArea()->m_hasInstallItemDesignations.at(*m_window.getFaction()).add(block, item, facing, *m_window.getFaction()); 
						hide(); 
					});
				}
			});
			// TODO: targeted haul.
		}
	}
}
