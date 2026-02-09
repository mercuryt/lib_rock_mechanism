#include "mainMenu.h"
#include "window.h"
#include "widgets.h"
#include <TGUI/Widgets/Group.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
MainMenuView::MainMenuView(Window& w) : m_window(w), m_panel(tgui::Panel::create())
{
	m_window.getGui().add(m_panel);
	update();
}
void MainMenuView::update()
{
	m_panel->removeAllWidgets();
	auto title = tgui::Label::create("Goblin Pit");
	title->setPosition("50%", "5%");
	title->setOrigin(0.5, 0);
	m_panel->add(title);
	auto layout = tgui::VerticalLayout::create();
	m_panel->add(layout);
	layout->setSize("60%", "70%");
	layout->setPosition("20%", "15%");
	/*
	auto quickStartButton = tgui::Button::create("quick start");
	quickStartButton->onClick([this]{ m_window.load("campaign/default"); });
	layout->add(quickStartButton);
	*/
	auto loadButton = tgui::Button::create("load");
	loadButton->onClick([this]{ m_window.m_editMode = false; m_window.showLoad(); });
	layout->add(loadButton);
	/*
	auto createWorldButton = tgui::Button::create("create world");
	createWorldButton->onClick([this]{ m_window.showCreateWorld(); });
	layout->add(createWorldButton);
	*/
	auto editCreateButton = tgui::Button::create("create and edit");
	layout->add(editCreateButton);
	editCreateButton->onClick([&]{ m_window.m_editMode = true; m_window.showEditReality(); });

	auto editLoadButton = tgui::Button::create("load and edit");
	layout->add(editLoadButton);
	editLoadButton->onClick([&]{ m_window.m_editMode = true; m_window.showLoad(); });

	if(m_window.getSimulation() != nullptr)
	{
		auto editButton = tgui::Button::create("edit");
		layout->add(editButton);
		editButton->onClick([&]{ m_window.m_editMode = true; m_window.showEditReality(); });
	}

	auto exitButton = tgui::Button::create("quit");
	exitButton->onClick([this]{ m_window.close(); });
	layout->add(exitButton);
}
