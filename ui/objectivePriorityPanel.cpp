#include "objectivePriorityPanel.h"
#include "window.h"
#include "../engine/actor.h"
#include "../engine/objectiveTypeInstances.h"
ObjectivePriorityView::ObjectivePriorityView(Window& w) : m_window(w), m_panel(tgui::Panel::create()), m_title(tgui::Label::create()), m_grid(tgui::Grid::create()), m_actor(nullptr)
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
	m_panel->add(m_title);
	m_panel->add(m_grid);
	m_grid->setPosition(20, tgui::bindBottom(m_title) + 20);
	size_t row = 1;
	for(auto& [name, type] : ObjectiveTypeInstances::objectiveTypes)
	{
		auto label = tgui::Label::create(name);
		m_grid->addWidget(label, row, 1);
		auto input = m_spinControls[type->getObjectiveTypeId()] = tgui::SpinControl::create();
		input->setMinimum(0);
		input->setMaximum(UINT8_MAX);
		ObjectiveType* typePointer = type.get();
		input->onValueChange([this, typePointer](float value){ m_actor->m_hasObjectives.m_prioritySet.setPriority(*typePointer, value); });
		m_grid->addWidget(input, row, 2);
		++row;
	}
	auto close = tgui::Button::create("close");
	m_panel->add(close);
	close->onClick([this]{ m_window.showGame(); });
	auto details = tgui::Button::create("details");
	m_panel->add(details);
	details->onClick([this]{ m_window.showActorDetail(*m_actor); });
}

void ObjectivePriorityView::draw(Actor& actor)
{
	m_actor = &actor;
	m_title->setText(L"Set priorities for " + m_actor->m_name);
	for(auto& pair : ObjectiveTypeInstances::objectiveTypes)
	{
		ObjectiveTypeId id = pair.second->getObjectiveTypeId();
		auto input = m_spinControls.at(id);
		input->setValue(actor.m_hasObjectives.m_prioritySet.getPriorityFor(id));
	}
	show();
}
