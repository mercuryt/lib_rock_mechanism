#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Label.hpp>
#include "contextMenu.h"
#include "infoPopup.h"
#include "../engine/types.h"
class Window;
class GameOverlay final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Panel::Ptr m_menu;
	InfoPopup m_infoPopup;
	ContextMenu m_contextMenu;
public:
	tgui::Label::Ptr m_x;
	tgui::Label::Ptr m_y;
	tgui::Label::Ptr m_z;
	tgui::Label::Ptr m_paused;
	Item* m_itemBeingInstalled;
	Facing m_facing;
	GameOverlay(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void drawContextMenu(Block& block) { m_contextMenu.draw(block); }
	void closeContextMenu() { m_contextMenu.hide(); }
	void drawMenu() { m_menu->setVisible(true); }
	void closeMenu() { m_menu->setVisible(false); }
	void drawInfoPopup(Block& block) { m_infoPopup.display(block); }
	void drawInfoPopup(Item& item) { m_infoPopup.display(item); }
	void drawInfoPopup(Plant& plant) { m_infoPopup.display(plant); }
	void drawInfoPopup(const Plant& plant) { drawInfoPopup(const_cast<Plant&>(plant)); }
	void drawInfoPopup(Actor& actor) { m_infoPopup.display(actor); }
	void closeInfoPopup() { m_infoPopup.hide(); };
	void installItem(Block& block);
	void unfocusUI();
	[[nodiscard]] bool isVisible() const { return m_group->isVisible(); }
	[[nodiscard]] bool menuIsVisible() const { return m_menu->isVisible(); }
	[[nodiscard]] bool contextMenuIsVisible() const { return m_contextMenu.isVisible(); }
	[[nodiscard]] bool infoPopupIsVisible() const { return m_infoPopup.isVisible(); }
	[[nodiscard]] tgui::Group::Ptr getGroup() const { return m_group; }
};