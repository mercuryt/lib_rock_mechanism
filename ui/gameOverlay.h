#include <TGUI/TGUI.hpp>
#include "detailPanel.h"
class Window;
class GameOverlay final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	DetailPanel m_detailPanel;
public:
	GameOverlay(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
};
