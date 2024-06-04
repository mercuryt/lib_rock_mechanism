#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../uiUtil.h"
#include "../../engine/block.h"
#include "../../engine/actor.h"
#include "../../engine/objectives/equipItem.h"
#include "../../engine/objectives/unequipItem.h"
#include "../../engine/simulation/hasItems.h"
#include <cmath>
#include <regex>
void ContextMenu::drawItemControls(BlockIndex& block)
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
					if(
							(m_window.m_editMode || m_window.getFaction() == actor.getFaction()) && 
							actor.m_equipmentSet.canEquipCurrently(*item)
					  )
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
				if(!actors.empty() && m_window.getFaction())
				{
					std::vector<const HasShape*> vector(actors.begin(), actors.end());
					if(vector.front()->m_canLead.getMoveSpeedForGroupWithAddedMass(vector, item->singleUnitMass()))
					{
						auto moveTo = tgui::Button::create("move to...");
						moveTo->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
						submenu.add(moveTo);
						moveTo->onClick([this, item]{ 
							m_window.getGameOverlay().m_itemBeingMoved = item;
							hide(); 
						});
					}
				}
				if(item->m_itemType.internalVolume)
				{
					auto addCargo = tgui::Label::create("add cargo");
					addCargo->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					submenu.add(addCargo);
					std::function<void(ItemParamaters params)> callback = [this, &item](ItemParamaters params){
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						Item& newItem = m_window.getSimulation()->m_hasItems->createItem(params);
						item->m_hasCargo.add(newItem);
						hide();
					};
					auto [itemTypeUI, materialTypeUI, quantityOrQualityLabel, quantityOrQualityUI, wearLabel, wearUI, confirmUI] = widgetUtil::makeCreateItemUI(callback);
					auto itemTypeLabel = tgui::Label::create("item type");
					itemTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					submenu.add(itemTypeLabel);
					submenu.add(itemTypeUI);
					auto materialTypeLabel = tgui::Label::create("material type");
					materialTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					submenu.add(materialTypeLabel);
					submenu.add(materialTypeUI);

					quantityOrQualityLabel->cast<tgui::Label>()->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					submenu.add(quantityOrQualityLabel);
					submenu.add(quantityOrQualityUI);
					wearLabel->cast<tgui::Label>()->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					submenu.add(wearLabel);
					submenu.add(wearUI);
					submenu.add(confirmUI);

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
		std::function<void(ItemParamaters params)> callback = [this, &block](ItemParamaters params){
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			if(m_window.getSelectedBlocks().empty())
				m_window.selectBlock(block);
			for(BlockIndex* selectedBlock : m_window.getSelectedBlocks())
			{
				static const MoveType& none = MoveType::byName("none");
				if(!selectedBlock->m_hasShapes.shapeAndMoveTypeCanEnterEverWithAnyFacing(params.itemType.shape, none))
					continue;
				params.location = selectedBlock;
				m_window.getSimulation()->m_hasItems->createItem(params);
			}
			hide();
		};
		auto addItem = tgui::Label::create("add item");
		addItem->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(addItem);
		addItem->onMouseEnter([this, callback]{
			auto& submenu = makeSubmenu(0);
			auto [itemTypeUI, materialTypeUI, quantityOrQualityLabel, quantityOrQualityUI, wearLabel, wearUI, confirmUI] = widgetUtil::makeCreateItemUI(callback);
			auto itemTypeLabel = tgui::Label::create("item type");
			itemTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(itemTypeLabel);
			submenu.add(itemTypeUI);
			auto materialTypeLabel = tgui::Label::create("material type");
			materialTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(materialTypeLabel);
			submenu.add(materialTypeUI);

			quantityOrQualityLabel->cast<tgui::Label>()->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(quantityOrQualityLabel);
			submenu.add(quantityOrQualityUI);
			wearLabel->cast<tgui::Label>()->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(wearLabel);
			submenu.add(wearUI);
			submenu.add(confirmUI);
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
