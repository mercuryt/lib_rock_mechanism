#include "actorPanel.h"
#include "window.h"
#include "../engine/actor.h"
#include <sys/select.h>
ActorView::ActorView(Window& window) : m_window(window), m_group(tgui::Group::create()), m_name(tgui::Label::create()), m_skills(tgui::Grid::create()), m_equipment(tgui::Grid::create()), m_wounds(tgui::Grid::create()), m_bleedRate(tgui::Label::create()), m_actor(nullptr)
{
	m_window.m_gui.add(m_group);
	m_group->add(tgui::Label::create("Actor Details"));
	m_group->add(m_name);
	// Skills.
	m_group->add(tgui::Label::create("skills"));
	auto scrollable = tgui::ScrollablePanel::create();
	scrollable->add(m_skills);
	m_group->add(scrollable);
	// Equipment.
	m_group->add(tgui::Label::create("equipment"));
	scrollable = tgui::ScrollablePanel::create();
	scrollable->add(m_equipment);
	m_group->add(scrollable);
	auto selectUniform = tgui::ComboBox::create();
	for(auto& pair : m_window.getSimulation()->m_hasUniforms.at(*m_window.getFaction()).getAll())
		selectUniform->addItem(pair.first);
	m_group->add(selectUniform);
	// Wounds.
	m_group->add(tgui::Label::create("wounds"));
	scrollable = tgui::ScrollablePanel::create();
	scrollable->add(m_wounds);
	m_group->add(scrollable);
	// Objective priority button.
	auto showObjectivePriority = tgui::Button::create("priorities");
	m_group->add(showObjectivePriority);
	showObjectivePriority->onClick([&]{ m_window.showObjectivePriority(*m_actor); });
	// Close button.
	auto close = tgui::Button::create("x");
	m_group->add(close);
	close->onClick([&]{ m_window.showGame(); });
}
void ActorView::draw(Actor& actor)
{
	m_actor = &actor;
	m_name->setText(m_actor->m_name);
	m_skills->removeAllWidgets();
	m_skills->addWidget(tgui::Label::create("name"), 1, 1);
	m_skills->addWidget(tgui::Label::create("level"), 2, 1);
	m_skills->addWidget(tgui::Label::create("xp"), 3, 1);
	uint32_t column = 2;
	for(auto& [skillType, skill] : m_actor->m_skillSet.m_skills)
	{
		m_skills->addWidget(tgui::Label::create(skillType->name), 1, column);
		m_skills->addWidget(tgui::Label::create(std::to_string(skill.m_level)), 2, column);
		auto xpText = std::to_string(skill.m_xp) + "/" + std::to_string(skill.m_xpForNextLevel);
		m_skills->addWidget(tgui::Label::create(xpText), 3, column);
		++column;
	}
	m_equipment->removeAllWidgets();
	m_equipment->addWidget(tgui::Label::create("type"), 1, 1);
	m_equipment->addWidget(tgui::Label::create("material"), 2, 1);
	m_equipment->addWidget(tgui::Label::create("quantity"), 3, 1);
	m_equipment->addWidget(tgui::Label::create("quality"), 4, 1);
	m_equipment->addWidget(tgui::Label::create("mass"), 5, 1);
	column = 2;
	for(Item* item : m_actor->m_equipmentSet.getAll())
	{
		m_equipment->addWidget(tgui::Label::create(item->m_itemType.name), 1, column);
		m_equipment->addWidget(tgui::Label::create(item->m_materialType.name), 2, column);
		// Only one of these will be used but they should not share a column.
		if(item->m_itemType.generic)
			m_equipment->addWidget(tgui::Label::create(std::to_string(item->getQuantity())), 3, column);
		else
			m_equipment->addWidget(tgui::Label::create(std::to_string(item->m_quality)), 4, column);
		m_equipment->addWidget(tgui::Label::create(std::to_string(item->getMass())), 5, column);
		if(!item->m_name.empty())
			m_equipment->addWidget(tgui::Label::create(item->m_name), 6, column);
		++column;
	}
	m_wounds->removeAllWidgets();
	m_wounds->addWidget(tgui::Label::create("body part"), 1, 1);
	m_wounds->addWidget(tgui::Label::create("area"), 2, 1);
	m_wounds->addWidget(tgui::Label::create("depth"), 3, 1);
	m_wounds->addWidget(tgui::Label::create("bleed"), 4, 1);
	column = 2;
	for(Wound* wound : m_actor->m_body.getAllWounds())
	{
		m_wounds->addWidget(tgui::Label::create(wound->bodyPart.bodyPartType.name), 1, 1);
		m_wounds->addWidget(tgui::Label::create(std::to_string(wound->hit.area)), 2, 1);
		m_wounds->addWidget(tgui::Label::create(std::to_string(wound->hit.depth)), 3, 1);
		m_wounds->addWidget(tgui::Label::create(std::to_string(wound->bleedVolumeRate)), 4, 1);
	}
	show();
}
