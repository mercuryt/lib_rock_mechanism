#include "objectivePriorityPanel.h"
#include "window.h"
#include "../engine/actor.h"
#include "../engine/objectiveTypeInstances.h"
#include <TGUI/Widgets/Group.hpp>
#include <TGUI/Widgets/SpinButton.hpp>
ObjectivePriorityView::ObjectivePriorityView(Window& w) : m_window(w), m_group(tgui::Group::create()), m_grid(tgui::Grid::create()), m_actorName(tgui::Label::create()), m_actor(nullptr)
{
	m_window.getGui().add(m_group);
	m_group->setVisible(false);
	auto title = tgui::Label::create("set objective priority");
	m_group->add(title);
	m_group->add(m_actorName);
	m_group->add(m_grid);
	std::size_t column = 1;
	for(auto& [name, type] : ObjectiveTypeInstances::objectiveTypes)
	{
		auto label = tgui::Label::create(name);
		m_grid->addWidget(label, 1, column);
		auto input = m_spinButtons[type->getObjectiveTypeId()] = tgui::SpinButton::create();
		input->setMinimum(0);
		input->setMaximum(UINT8_MAX);
		input->onValueChange([&](float value){ m_actor->m_hasObjectives.m_prioritySet.setPriority(*type.get(), value); });
		m_grid->addWidget(input, 2, column);
		++column;
	}
}

void ObjectivePriorityView::draw(Actor& actor)
{
	m_actor = &actor;
	m_actorName->setText(m_actor->m_name);
	for(auto& pair : ObjectiveTypeInstances::objectiveTypes)
	{
		ObjectiveTypeId id = pair.second->getObjectiveTypeId();
		tgui::SpinButton::Ptr input = m_spinButtons.at(id);
		input->setValue(actor.m_hasObjectives.m_prioritySet.getPriorityFor(id));
	}
	show();
}
