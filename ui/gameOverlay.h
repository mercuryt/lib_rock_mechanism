#include <TGUI/TGUI.hpp>
#include "contextMenu.h"
#include "detailPanel.h"
class Window;
class GameOverlay final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Panel::Ptr m_menu;
	DetailPanel m_detailPanel;
	ContextMenu m_contextMenu;
public:
	GameOverlay(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void drawContextMenu(Block& block) { m_contextMenu.draw(block); }
	void closeContextMenu() { m_contextMenu.hide(); }
	void drawMenu() { m_menu->setVisible(true); }
	void closeMenu() { m_menu->setVisible(false); }
	void closeDetailPanel() { m_detailPanel.hide(); };
	[[nodiscard]] bool isVisibile() const { return m_group->isVisible(); }
	[[nodiscard]] bool menuIsVisibile() const { return m_menu->isVisible(); }
	[[nodiscard]] bool contextMenuIsVisible() const { return m_contextMenu.isVisible(); }
	[[nodiscard]] bool detailPanelIsVisible() const { return m_detailPanel.isVisible(); }
};
