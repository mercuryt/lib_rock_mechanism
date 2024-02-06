#include "gameOverlay.h"
#include "window.h"
#include <TGUI/Widgets/HorizontalLayout.hpp>
GameOverlay::GameOverlay(Window& w) : m_window(w), 
	m_group(tgui::Group::create()), m_menu(tgui::Panel::create()), m_infoPopup(w), m_contextMenu(w, m_group), 
	m_x(tgui::Label::create()), m_y(tgui::Label::create()), m_z(tgui::Label::create()), m_paused(tgui::Label::create("paused")),
	m_itemBeingInstalled(nullptr), m_facing(0) 
{ 
	m_window.getGui().add(m_group);
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
	auto topBar = tgui::HorizontalLayout::create();
	auto leftSide = tgui::Group::create();
	topBar->add(leftSide);
	auto coordinateHolder = tgui::HorizontalLayout::create();
	coordinateHolder->setWidth("10%");
	leftSide->add(coordinateHolder);
	coordinateHolder->add(m_x);
	coordinateHolder->add(m_y);
	coordinateHolder->add(m_z);
	topBar->setHeight("5%");
	topBar->add(m_paused);
	m_group->add(topBar);
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
