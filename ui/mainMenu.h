#include <TGUI/TGUI.hpp>
class Window;
class MainMenuView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
public:
	MainMenuView(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
};
