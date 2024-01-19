#include "mainMenu.h"
#include "window.h"
#include <TGUI/Widgets/Group.hpp>
MainMenuView::MainMenuView(Window& w) : m_window(w), m_panel(tgui::Panel::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setAutoLayout(tgui::AutoLayout::Fill);
	auto title = tgui::Label::create("Goblin Pit");
	m_panel->add(title);
	auto quickStartButton = tgui::Button::create("quick start");
	quickStartButton->onClick([this]{ m_window.load("campaign/default"); });
	m_panel->add(quickStartButton);
	auto loadButton = tgui::Button::create("load");
	loadButton->onClick([this]{ m_window.showLoad(); });
	m_panel->add(loadButton);
	auto createWorld = tgui::Button::create("create world");
	loadButton->onClick([this]{ m_window.showCreateWorld(); });
	m_panel->add(loadButton);
	auto exitButton = tgui::Button::create("quit");
	exitButton->onClick([this]{ m_window.close(); });
	m_panel->add(exitButton);
}
