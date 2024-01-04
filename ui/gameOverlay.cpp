#include "gameOverlay.h"
#include "window.h"
GameOverlay::GameOverlay(Window& w) : m_window(w), m_group(tgui::Group::create()), m_detailPanel(w, m_group)
{ 
	m_window.m_gui.add(m_group);
	m_group->setVisible(false);
}
