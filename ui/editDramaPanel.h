#pragma once
#include <TGUI/TGUI.hpp>
class Window;
class Area;
class EditDramaView final
{
	Window& m_window;
	tgui::Group::Ptr m_panel;
public:
	EditDramaView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	// Nullptr area means global drama.
	void draw(Area* area = nullptr);
};
