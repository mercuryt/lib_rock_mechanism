#include "editAreaPanel.h"
#include "window.h"
#include <TGUI/Widgets/Group.hpp>
#include <TGUI/Widgets/HorizontalLayout.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
EditAreaView::EditAreaView(Window& w) : m_window(w), m_panel(tgui::Panel::create()),
	m_title(tgui::Label::create()), 
	m_name(tgui::EditBox::create()), 
	m_dimensionsHolder(tgui::Grid::create()), 
	m_sizeX(tgui::SpinControl::create()), m_sizeY(tgui::SpinControl::create()), m_sizeZ(tgui::SpinControl::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
	m_title->setPosition("50%", "5%");
	m_title->setOrigin(0.5, 0);
	m_panel->add(m_title);
	auto layout = tgui::VerticalLayout::create();
	widgetUtil::setPadding(layout);
	m_panel->add(layout);
	

	layout->add(tgui::Label::create("name"));
	layout->add(m_name);

	layout->add(m_dimensionsHolder);
	m_dimensionsHolder->addWidget(tgui::Label::create("dimensions"), 1, 1);
	m_dimensionsHolder->addWidget(tgui::Label::create("x"), 2, 1);
	m_dimensionsHolder->addWidget(tgui::Label::create("y"), 3, 1);
	m_dimensionsHolder->addWidget(tgui::Label::create("z"), 4, 1);
	m_dimensionsHolder->addWidget(m_sizeX, 1, 2);
	m_dimensionsHolder->addWidget(m_sizeY, 2, 2);
	m_dimensionsHolder->addWidget(m_sizeZ, 3, 2);
	m_sizeX->setMinimum(10);
	m_sizeX->setMaximum(2000);
	m_sizeX->setValue(100);
	m_sizeY->setMinimum(10);
	m_sizeY->setMaximum(2000);
	m_sizeY->setValue(100);
	m_sizeZ->setMinimum(10);
	m_sizeZ->setMaximum(2000);
	m_sizeZ->setValue(50);

	auto buttonLayout = tgui::HorizontalLayout::create();
	layout->add(buttonLayout);

	auto confirmButton = tgui::Button::create("confirm");
	confirmButton->onClick([&]{ confirm(); });
	buttonLayout->add(confirmButton);

	auto exitButton = tgui::Button::create("back");
	exitButton->onClick([this]{ m_window.showEditReality(); });
	buttonLayout->add(exitButton);
}
void EditAreaView::draw(Area* area)
{
	if(area)
	{
		m_title->setText(L"Edit " + area->m_name);
		m_dimensionsHolder->setVisible(false);
		m_area = area;
	}
	else
	{
		m_title->setText(L"Create area");
		m_dimensionsHolder->setVisible(true);
		m_area = nullptr;
	}
	show();
}
void EditAreaView::confirm()
{
	if(!m_area)
	{
		std::function<void()> task = [this]{
			m_area = &m_window.getSimulation()->createArea(m_sizeX->getValue(), m_sizeY->getValue(), m_sizeZ->getValue());
		};
		m_window.threadTask(task);
	}
	m_area->m_name = m_name->getText().toWideString();
	m_window.showEditReality();
}
