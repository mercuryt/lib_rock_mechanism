#include "gameOverlay.h"
#include "window.h"
#include "uiUtil.h"
#include "../engine/config.h"
#include <TGUI/Widgets/Panel.hpp>
GameOverlay::GameOverlay(Window& w) : m_window(w),
	m_group(tgui::Group::create()), m_menu(tgui::Panel::create()), m_infoPopup(w), m_contextMenu(w, m_group),
	m_coordinateUI(tgui::Label::create()), m_zoomUI(tgui::Label::create()), m_timeUI(tgui::Label::create()),
	m_speedUI(tgui::Label::create()), m_weatherUI(tgui::Label::create()),
	m_selectionStatusUI(tgui::Label::create())
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
	topBar->add(m_zoomUI);
	m_zoomUI->setPosition("5%", 0);
	topBar->add(m_timeUI);
	m_timeUI->setPosition("10%", 0);
	topBar->add(m_speedUI);
	m_speedUI->setPosition("40%", 0);
	topBar->add(m_weatherUI);
	m_weatherUI->setPosition("70%", 0);
	m_group->add(topBar);
	// Selection status.
	m_selectionStatusUI->setPosition(0, 20);
	m_group->add(m_selectionStatusUI);
}
void GameOverlay::drawMenu()
{
	m_window.setPaused(true);
	m_menu->setVisible(true);
}
void GameOverlay::assignLocationToInstallItem(const BlockIndex& block)
{
	assert(m_itemBeingInstalled.exists());
	Area& area = *m_window.getArea();
	area.m_hasInstallItemDesignations.getForFaction(m_window.getFaction()).add(area, block, m_itemBeingInstalled, m_facing, m_window.getFaction());
	m_itemBeingInstalled.clear();
}
void GameOverlay::assignLocationToMoveItemTo(const BlockIndex& block)
{
	assert(m_itemBeingMoved.exists());
	assert(!m_window.getSelectedActors().empty());
	m_window.getArea()->m_hasTargetedHauling.begin(
		m_window.getSelectedActors(),
		m_itemBeingMoved,
		block
	);
	m_itemBeingMoved.clear();
}
void GameOverlay::unfocusUI()
{
	// Find whatever widget has focus and defocus it. Apperently TGUI doesn't defocus widgets when they get hidden?
	tgui::Widget::Ptr focused = m_window.getGui().getFocusedChild();
	if(focused)
		focused->setFocused(false);
}
void GameOverlay::drawZoom()
{
	float ratio = (float)m_window.getScale() / 32.f;
	std::wstringstream stream;
	stream << std::fixed << std::setprecision(2) << ratio;
	m_zoomUI->setText(L"zoom: " + UIUtil::floatToString(ratio));
}
void GameOverlay::drawWeatherReport()
{
	Area& area = *m_window.getArea();
	std::wstring text = UIUtil::temperatureToString(area.m_hasTemperature.getAmbientSurfaceTemperature());
	if(area.m_hasRain.isRaining())
		text += L"rain : " + std::to_wstring(area.m_hasRain.getIntensityPercent().get()) + L"%";
}
void GameOverlay::drawTime()
{
	DateTime now = m_window.getSimulation()->getDateTime();
	// TODO: getting seconds and minutes from hourlyEvent.percentComplete is roundabout and wierd, add them to DateTime instead?
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
void GameOverlay::drawSelectionDescription()
{
	std::wstring description;
	switch(m_window.getSelectMode())
	{
		case(SelectMode::Actors):
			description = L"actors: " + std::to_wstring(m_window.getSelectedActors().size());
			break;
		case(SelectMode::Items):
			description = L"items: " + std::to_wstring(m_window.getSelectedItems().size());
			break;
		case(SelectMode::Plants):
			description = L"plants: " + std::to_wstring(m_window.getSelectedPlants().size());
			break;
		case(SelectMode::Blocks):
			description = L"blocks: " + std::to_wstring(m_window.getSelectedBlocks().size());
			break;
		default:
			std::unreachable();
	}
	m_selectionStatusUI->setText(description);
}
