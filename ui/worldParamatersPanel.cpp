#include "worldParamatersPanel.h"
#include "window.h"
#include <TGUI/Widgets/Group.hpp>
WorldParamatersView::WorldParamatersView(Window& w) : m_window(w), m_panel(tgui::Panel::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setAutoLayout(tgui::AutoLayout::Fill);
	auto title = tgui::Label::create("Create World");
	m_panel->add(title);
	auto elevationLociiCountLabel = tgui::Label::create("elevation count");
	m_panel->add(elevationLociiCountLabel);
	auto exitButton = tgui::Button::create("quit");
	exitButton->onClick([this]{ m_window.close(); });
	m_panel->add(exitButton);
}
