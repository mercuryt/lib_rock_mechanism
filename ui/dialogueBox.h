#pragma once
#include <TGUI/Widgets/ChildWindow.hpp>
class Window;

class DialogueBoxUI
{
	Window& m_window;
	tgui::ChildWindow::Ptr m_childWindow;
public:
	DialogueBoxUI(Window& window) : m_window(window) { }
	void draw();
	void hide();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool isVisible() const;
};
