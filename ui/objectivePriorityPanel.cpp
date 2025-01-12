#include "objectivePriorityPanel.h"
#include "window.h"
#include "../engine/actors/actors.h"
#include <TGUI/Layout.hpp>
ObjectivePriorityView::ObjectivePriorityView(Window& w) : m_window(w), m_panel(tgui::Panel::create()), m_title(tgui::Label::create()), m_grid(tgui::Grid::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
	m_panel->add(m_title);
	m_panel->add(m_grid);
	m_grid->setPosition(20, tgui::bindBottom(m_title) + 20);
	size_t row = 1;
	for(auto& objectiveType : objectiveTypeData)
	{
		auto label = tgui::Label::create(objectiveType->name());
		m_grid->addWidget(label, row, 1);
		auto input = m_spinControls[objectiveType->getId()] = tgui::SpinControl::create();
		input->setMinimum(0);
		input->setMaximum(UINT8_MAX);
		input->onValueChange([this, &objectiveType](float value){
			Actors& actors = m_window.getArea()->getActors();
			actors.objective_setPriority(m_actor, objectiveType->getId(), Priority::create(value));
		});
		m_grid->addWidget(input, row, 2);
		++row;
	}
	auto close = tgui::Button::create("close");
	close->setPosition(10, tgui::bindBottom(m_grid) + 10);
	m_panel->add(close);
	close->onClick([this]{ m_window.showGame(); });
	auto details = tgui::Button::create("details");
	details->setPosition(tgui::bindRight(close) + 10, tgui::bindBottom(m_grid) + 10);
	m_panel->add(details);
	details->onClick([this]{ m_window.showActorDetail(m_actor); });
}

void ObjectivePriorityView::draw(const ActorIndex& actor)
{
	Actors& actors = m_window.getArea()->getActors();
	m_actor = actor;
	m_title->setText(L"Set priorities for " + actors.getName(m_actor));
	for(auto& objectiveType : objectiveTypeData)
	{
		ObjectiveTypeId id = objectiveType->getId();
		auto input = m_spinControls.at(id);
		input->setValue(actors.objective_getPriorityFor(actor, id).get());
	}
	show();
}
