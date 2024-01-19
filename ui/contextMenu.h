#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widget.hpp>
#include <TGUI/Widgets/Panel.hpp>
class Window;
class Block;
class ContextMenu final
{
	Window& m_window;
	tgui::Panel::Ptr m_group;
	tgui::Group::Ptr m_gameOverlayGroup;
	std::vector<tgui::Panel::Ptr> m_submenus;
public:
	ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup);
	void draw(Block& block);
	void hide();
	tgui::Panel::Ptr makeSubmenu(size_t index, tgui::Widget::Ptr creator);
	[[nodiscard]] bool isVisible() const { return m_group->isVisible(); }
};
