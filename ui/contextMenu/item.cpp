#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../uiUtil.h"
#include "../../engine/space/space.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/objectives/equipItem.h"
#include "../../engine/objectives/unequipItem.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/definitions/moveType.h"
#include "../../engine/hasShapes.hpp"
#include <cmath>
#include <regex>
void ContextMenu::drawItemControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	const FactionId& faction = m_window.getFaction();
	// Item submenu.
	if(!space.item_empty(point))
		for(const ItemIndex& item : space.item_getAll(point))
		{
			auto label = tgui::Label::create(m_window.displayNameForItem(item));
			m_root.add(label);
			label->onMouseEnter([this, item, faction, &items, &actors]{
				auto& submenu = makeSubmenu(0);
				auto info = tgui::Button::create("info");
				submenu.add(info);
				info->onClick([this, item]{ m_window.getGameOverlay().drawInfoPopup(item); });
				if(faction.exists())
				{
					bool itemAtClickedLocationMarked = items.stockpile_canBeStockPiled(item, faction);
					auto stockpileUI = tgui::Button::create(itemAtClickedLocationMarked ? "do not stockpile" : "stockpile");
					submenu.add(stockpileUI);
					stockpileUI->onClick([item, itemAtClickedLocationMarked, this, faction, &items]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						if(m_window.getSelectedItems().empty())
							m_window.selectItem(item);
						for(const ItemIndex& selectedItem : m_window.getSelectedItems())
						{
							bool selectedItemMarked = items.stockpile_canBeStockPiled(selectedItem, faction);
							if(!itemAtClickedLocationMarked)
							{
								if(!selectedItemMarked)
									m_window.getArea()->m_hasStockPiles.getForFaction(faction).addItem(selectedItem);
							}
							else if(selectedItemMarked)
								m_window.getArea()->m_hasStockPiles.getForFaction(faction).removeItem(selectedItem);
						}
						hide();
					});
				}
				//TODO: targeted haul, installation , stockpile
				if(m_window.m_editMode)
				{
					auto destroy = tgui::Button::create("destroy");
					submenu.add(destroy);
					destroy->onClick([this, item, &items]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						m_window.deselectAll();
						items.destroy(item);
						hide();
					});
				}
				auto& selectedActors = m_window.getSelectedActors();
				if(selectedActors.size() == 1)
				{
					const ActorIndex& actor = *selectedActors.begin();
					// Equip.
					if(
							(m_window.m_editMode || m_window.getFaction() == actors.getFaction(actor)) &&
							actors.equipment_canEquipCurrently(actor, item)
					  )
					{
						auto button = tgui::Button::create("equip");
						button->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
						submenu.add(button);
						button->onClick([this, &actors, &items, actor, item] () mutable {
							std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
							ItemReference itemRef = items.getReference(item);
							std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(itemRef);
							actors.objective_replaceTasks(actor, std::move(objective));
							hide();
						});
					}
				}
				if(!selectedActors.empty() && faction.exists())
				{
					if(actors.lineLead_getSpeedWithAddedMass(selectedActors, items.getSingleUnitMass(item)) != 0)
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
				if(ItemType::getInternalVolume(items.getItemType(item)) != 0)
				{
					auto addCargo = tgui::Label::create("add cargo");
					addCargo->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
					submenu.add(addCargo);
					std::function<void(ItemParamaters params)> callback = [this, &items, item](ItemParamaters params){
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						const ItemIndex& newItem = items.create(params);
						items.cargo_addItem(item, newItem, items.getQuantity(newItem));
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
	auto& selectedActors = m_window.getSelectedActors();
	if(selectedActors.size() == 1)
	{
		const ActorIndex& actor = selectedActors.front();
		auto& equipment = actors.equipment_getAll(actor);
		if(!equipment.empty())
		{
			auto unequip = tgui::Label::create("unequip");
			m_root.add(unequip);
			unequip->onMouseEnter([this, point, actor, &equipment, &items, &actors, &area]{
				auto& submenu = makeSubmenu(0);
				for(const ItemReference& itemRef : equipment)
				{
					const ItemIndex& item = itemRef.getIndex(items.m_referenceData);
					auto button = tgui::Label::create(UIUtil::describeItem(area, item));
					submenu.add(button);
					button->onClick([this, point, actor, item, itemRef, &actors]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						std::unique_ptr<Objective> objective = std::make_unique<UnequipItemObjective>(itemRef, point);
						actors.objective_replaceTasks(actor, std::move(objective));
						hide();
					});
				}
			});
		}
	}
	if(m_window.m_editMode)
	{
		std::function<void(ItemParamaters params)> callback = [this, &space, &items, point](ItemParamaters params){
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			if(m_window.getSelectedBlocks().empty())
				m_window.selectBlock(point);
			for(const Point3D& selectedBlock : m_window.getSelectedBlocks().getView(space))
			{
				static const MoveTypeId& none = MoveType::byName(L"none");
				const ShapeId& shape = ItemType::getShape(params.itemType);
				if(!space.shape_shapeAndMoveTypeCanEnterEverWithAnyFacing(selectedBlock, shape, none))
					continue;
				params.location = selectedBlock;
				items.create(params);
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
	auto selectedItems = m_window.getSelectedItems();
	if(selectedItems.size() == 1)
	{
		const ItemIndex& item = selectedItems.front();
		if(ItemType::getInstallable(items.getItemType(item)))
		{
			// Install item.
			auto install = tgui::Label::create("install...");
			install->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			m_root.add(install);
			install->onMouseEnter([this, point, item, faction, &area]{
				auto& submenu = makeSubmenu(0);
				for(Facing4 facing = Facing4::North; facing != Facing4::Null; facing = Facing4(uint(facing) + 1))
				{
					auto button = tgui::Button::create(m_window.facingToString(facing));
					button->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
					submenu.add(button);
					button->onClick([this, point, item, facing, faction, &area]{
						std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
						m_window.getArea()->m_hasInstallItemDesignations.getForFaction(faction).add(area, point, item, facing, faction);
						hide();
					});
				}
			});
			// TODO: targeted haul.
		}
	}
}
