#include "editRealityPanel.h"
#include "datetime.h"
#include "simulation/simulation.h"
#include "widgets.h"
#include "window.h"
#include <TGUI/Vertex.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
#include <sys/select.h>
EditRealityView::EditRealityView(Window& w) : m_window(w), m_panel(tgui::Panel::create()), m_title(tgui::Label::create()), m_name(tgui::EditBox::create()), m_areaHolder(tgui::VerticalLayout::create()), m_confirm(tgui::Button::create()), m_areaEdit(tgui::Button::create("edit")), m_areaView(tgui::Button::create("view"))
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
	m_title->setPosition("50%", "5%");
	m_title->setOrigin(0.5, 0);
	m_panel->add(m_title);
	auto layout = tgui::VerticalLayout::create();
	widgetUtil::setPadding(layout);
	m_panel->add(layout);

	auto grid = tgui::Grid::create();
	layout->add(grid);

	grid->addWidget(tgui::Label::create("name"), 1, 1);
	grid->addWidget(m_name, 1, 2);

	grid->addWidget(tgui::Label::create("date and time"), 2, 1);
	grid->addWidget(m_dateTime.m_widget, 2, 2);

	layout->add(m_areaHolder);
	m_areaHolder->add(tgui::Label::create("areas"));
	m_areaHolder->add(m_areaSelect.m_widget);
	auto areaButtonLayout = tgui::HorizontalLayout::create();
	m_areaHolder->add(areaButtonLayout);
	auto create = tgui::Button::create("create");
	areaButtonLayout->add(create);
	areaButtonLayout->add(m_areaEdit);
	areaButtonLayout->add(m_areaView);
	create->onClick([&]{ m_window.showEditArea(); });
	m_areaEdit->onClick([&]{ m_window.showEditArea(&m_areaSelect.get()); });
	m_areaView->onClick([&]{
		m_window.setArea(m_areaSelect.get());
		m_window.showGame();
	});
	auto buttonLayout = tgui::HorizontalLayout::create();
	layout->add(buttonLayout);

	m_confirm->onClick([&]{
		if(m_name->getText().empty())
			return;
		if(!m_window.getSimulation())
			m_window.setSimulation(std::make_unique<Simulation>(m_name->getText().toWideString(), m_dateTime.get()));
		else
			m_window.getSimulation()->m_name = m_name->getText().toWideString();
		m_window.getSimulation()->save();
		draw();
	});
	buttonLayout->add(m_confirm);
	auto exitButton = tgui::Button::create("back");
	exitButton->onClick([&]{ m_window.showMainMenu(); });
	buttonLayout->add(exitButton);
	m_areaSelect.m_widget->onItemSelect([&]{ m_areaEdit->setEnabled(true); m_areaView->setEnabled(true); });
}
void EditRealityView::draw()
{
	Simulation* simulation = m_window.getSimulation();
	m_title->setText(simulation ? L"Edit " + simulation->m_name : L"Create reality");
	m_name->setText(simulation ? simulation->m_name : L"");
	m_dateTime.set(simulation ? simulation->getDateTime() : DateTime{12, 150, 1200});
	m_areaHolder->setVisible(simulation);
	m_confirm->setText(simulation ? "save" : "create");
	if(simulation)
		m_areaSelect.populate(*m_window.getSimulation());
	if(!m_areaSelect.hasSelection())
	{
		m_areaEdit->setEnabled(false);
		m_areaView->setEnabled(false);
	}
	show();
}
