#include "gameOverlay.h"
#include "config.h"
#include "util.h"
#include "window.h"
#include <TGUI/Widgets/Panel.hpp>
GameOverlay::GameOverlay(Window& w) : m_window(w), 
	m_group(tgui::Group::create()), m_menu(tgui::Panel::create()), m_infoPopup(w), m_contextMenu(w, m_group), 
	m_coordinateUI(tgui::Label::create()), m_timeUI(tgui::Label::create()), m_speedUI(tgui::Label::create()), m_weatherUI(tgui::Label::create()),
	m_itemBeingInstalled(nullptr), m_facing(0) 
{ 
	m_window.getGui().add(m_group);
	m_group->setVisible(false);
	// Game overlay menu.
	m_group->add(m_menu);
	auto layout = tgui::VerticalLayout::create();
	auto save = tgui::Button::create("save");
	layout->add(save);
	save->onClick([&]{ m_window.save(); });
	auto mainMenu = tgui::Button::create("main menu");
	layout->add(mainMenu);
	mainMenu->onClick([&]{ closeMenu(); m_window.showMainMenu(); });
	auto close = tgui::Button::create("close");
	layout->add(close);
	close->onClick([&]{ closeMenu(); });
	//TODO: settings.
	m_menu->add(layout);
	m_menu->setSize(120, 90);
	m_menu->setVisible(false);
	layout->setPosition(4, 4);
	layout->setSize(tgui::bindWidth(m_menu) - 8, tgui::bindHeight(m_menu) - 8);
	m_menu->setPosition("50%", "50%");
	m_menu->setOrigin(0.5, 0.5);

	// Top bar.
	auto topBar = tgui::Panel::create();
	topBar->setHeight(16);
	topBar->add(m_coordinateUI);
	topBar->add(m_timeUI);
	m_timeUI->setPosition("20%", 0);
	topBar->add(m_speedUI);
	m_speedUI->setPosition("40%", 0);
	topBar->add(m_weatherUI);
	m_weatherUI->setPosition("60%", 0);
	m_group->add(topBar);
}
void GameOverlay::drawMenu()
{ 
	m_window.setPaused(true); 
	m_menu->setVisible(true);
}
void GameOverlay::installItem(Block& block)
{
	assert(m_itemBeingInstalled);
	m_window.getArea()->m_hasInstallItemDesignations.at(*m_window.getFaction()).add(block, *m_itemBeingInstalled, m_facing, *m_window.getFaction());
	m_itemBeingInstalled = nullptr;
}
void GameOverlay::unfocusUI()
{
	// Find whatever widget has focus and defocus it. Apperently TGUI doesn't defocus widgets when they get destroyed?
	tgui::Widget::Ptr focused = m_window.getGui().getFocusedChild();
	if(focused)
		focused->setFocused(false);
}
void GameOverlay::drawTime()
{
	DateTime& now = m_window.getSimulation()->m_now;
	float fractionOfCurrentHourElapsed = m_window.getSimulation()->m_hourlyEvent.fractionComplete();
	uint16_t seconds = Config::minutesPerHour * Config::secondsPerMinute * fractionOfCurrentHourElapsed;
	uint16_t minutes = seconds / Config::secondsPerMinute;
	seconds -= minutes * Config::secondsPerMinute;
	m_timeUI->setText(
			"seconds: " + std::to_string(seconds) + 
			" minutes: " + std::to_string(minutes) + 
			" hour: " + std::to_string(now.hour) + 
			" day: " + std::to_string(now.day) + 
			" year: " + std::to_string(now.year)
	);
}
