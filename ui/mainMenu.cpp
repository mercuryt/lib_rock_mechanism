#include "mainMenu.h"
#include "window.h"
MainMenuView::MainMenuView(Window& w) : m_window(w), m_group(tgui::Panel::create())
{
	m_window.m_gui.add(m_group);
	auto title = tgui::Label::create("Goblin Pit");
	m_group->add(title);
	auto quickStartButton = tgui::Button::create("quick start");
	quickStartButton->onPress([this]{ m_window.load("campaign/default"); });
	m_group->add(quickStartButton);
	auto loadButton = tgui::Button::create("load");
	loadButton->onPress([this]{ m_window.showLoad(); });
	m_group->add(loadButton);
}
