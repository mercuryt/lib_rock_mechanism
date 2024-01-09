#include "gameOverlay.h"
#include "window.h"
GameOverlay::GameOverlay(Window& w) : m_window(w), m_group(tgui::Group::create()), m_menu(tgui::Panel::create()), m_detailPanel(w, m_group), m_contextMenu(w, m_group)
{ 
	m_window.m_gui.add(m_group);
	m_group->setVisible(false);
	m_group->add(m_menu);
	m_menu->setVisible(false);
	auto save = tgui::Button::create("save");
	m_menu->add(save);
	save->onClick([&]{ m_window.save(); });
	auto mainMenu = tgui::Button::create("main menu");
	m_menu->add(mainMenu);
	mainMenu->onClick([&]{
		m_group->setVisible(false);
	       	m_window.showMainMenu(); 
	});
	auto close = tgui::Button::create("close");
	m_menu->add(close);
	close->onClick([&]{ m_group->setVisible(false); });
	//TODO: settings.
	m_menu->setPosition("50%", "50%");
}
