#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Label.hpp>
#include "contextMenu.h"
#include "infoPopup.h"
#include "../engine/types.h"
#include "../engine/index.h"
class Window;
class GameOverlay final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Panel::Ptr m_menu;
	InfoPopup m_infoPopup;
	ContextMenu m_contextMenu;
public:
	tgui::Label::Ptr m_coordinateUI;
	tgui::Label::Ptr m_timeUI;
	tgui::Label::Ptr m_speedUI;
	tgui::Label::Ptr m_weatherUI;
	ItemIndex m_itemBeingInstalled;
	// TODO: Change to shape being moved for targeted hauling of downed actors.
	ItemIndex m_itemBeingMoved;
	Facing4 m_facing;
	GameOverlay(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void drawContextMenu(const BlockIndex& block) { m_contextMenu.draw(block); }
	void closeContextMenu() { m_contextMenu.hide(); }
	void drawMenu();
	void closeMenu() { m_menu->setVisible(false); }
	void drawInfoPopup(const BlockIndex& block) { m_infoPopup.display(block); }
	void drawInfoPopup(const ItemIndex& item) { m_infoPopup.display(item); }
	void drawInfoPopup(const PlantIndex& plant) { m_infoPopup.display(plant); }
	void drawInfoPopup(const ActorIndex& actor) { m_infoPopup.display(actor); }
	void closeInfoPopup() { m_infoPopup.hide(); };
	void assignLocationToInstallItem(const BlockIndex& block);
	void assignLocationToMoveItemTo(const BlockIndex& block);
	void unfocusUI();
	void drawTime();
	void updateInfoPopup() { m_infoPopup.update(); }
	[[nodiscard]] bool isVisible() const { return m_group->isVisible(); }
	[[nodiscard]] bool menuIsVisible() const { return m_menu->isVisible(); }
	[[nodiscard]] bool contextMenuIsVisible() const { return m_contextMenu.isVisible(); }
	[[nodiscard]] bool infoPopupIsVisible() const { return m_infoPopup.isVisible(); }
	[[nodiscard]] tgui::Group::Ptr getGroup() const { return m_group; }
};
