#include "actorPanel.h"
#include "displayData.h"
#include "window.h"
#include "../engine/actor.h"
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
#include <regex>
#include <string>
ActorView::ActorView(Window& window) : m_window(window), m_panel(tgui::ScrollablePanel::create()), m_actor(nullptr)
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}
void ActorView::draw(Actor& actor)
{
	m_actor = &actor;
	m_panel->removeAllWidgets();
	auto layout = tgui::VerticalLayout::create();
	layout->setPosition("5%", "5%");
	layout->setSize("90%", "90%");
	layout->add(tgui::Label::create("Actor Details"));
	layout->add(tgui::Label::create(L"name: " + actor.m_name));
	layout->add(tgui::Label::create("age : " + std::to_string(actor.getAgeInYears())));
	// Attributes.
	layout->add(tgui::Label::create("attributes"));
	auto attributesGrid = tgui::Grid::create();
	attributesGrid->addWidget(tgui::Label::create("strength"), 1, 1);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(m_actor->m_attributes.getStrength())), 2, 1);
	attributesGrid->addWidget(tgui::Label::create("dextarity"), 1, 2);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(m_actor->m_attributes.getDextarity())), 2, 2);
	attributesGrid->addWidget(tgui::Label::create("agility"), 1, 3);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(m_actor->m_attributes.getAgility())), 2, 3);
	attributesGrid->addWidget(tgui::Label::create("unencombered carry mass"), 1, 4);
	attributesGrid->addWidget(tgui::Label::create(displayData::localizeNumber(m_actor->m_attributes.getUnencomberedCarryMass())), 2, 4);
	attributesGrid->addWidget(tgui::Label::create("move speed"), 1, 5);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(m_actor->m_attributes.getMoveSpeed())), 2, 5);
	attributesGrid->addWidget(tgui::Label::create("base combat score"), 1, 6);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(m_actor->m_attributes.getBaseCombatScore())), 2, 6);
	layout->add(attributesGrid);
	// Skills.
	if(!m_actor->m_skillSet.m_skills.empty())
	{
		layout->add(tgui::Label::create("skills"));
		auto skillsGrid = tgui::Grid::create();
		uint32_t skillColumn = 2;
		skillsGrid->addWidget(tgui::Label::create("name"), 1, 1);
		skillsGrid->addWidget(tgui::Label::create("level"), 2, 1);
		skillsGrid->addWidget(tgui::Label::create("xp"), 3, 1);
		for(auto& [skillType, skill] : m_actor->m_skillSet.m_skills)
		{
			skillsGrid->addWidget(tgui::Label::create(skillType->name), 1, skillColumn);
			skillsGrid->addWidget(tgui::Label::create(std::to_string(skill.m_level)), 2, skillColumn);
			auto xpText = std::to_string(skill.m_xp) + "/" + std::to_string(skill.m_xpForNextLevel);
			skillsGrid->addWidget(tgui::Label::create(xpText), 3, skillColumn);
			++skillColumn;
		}
		layout->add(skillsGrid);
	}
	// Equipment.
	if(!m_actor->m_equipmentSet.getAll().empty())
	{
		layout->add(tgui::Label::create("equipment"));
		auto equipmentGrid = tgui::Grid::create();
		auto uniformUI = tgui::ComboBox::create();
		equipmentGrid->addWidget(tgui::Label::create("no equipment"), 1, 1);
		equipmentGrid->addWidget(tgui::Label::create("type"), 1, 1);
		equipmentGrid->addWidget(tgui::Label::create("material"), 2, 1);
		equipmentGrid->addWidget(tgui::Label::create("quantity"), 3, 1);
		equipmentGrid->addWidget(tgui::Label::create("quality"), 4, 1);
		equipmentGrid->addWidget(tgui::Label::create("mass"), 5, 1);
		uint8_t column = 2;
		for(Item* item : m_actor->m_equipmentSet.getAll())
		{
			equipmentGrid->addWidget(tgui::Label::create(item->m_itemType.name), 1, column);
			equipmentGrid->addWidget(tgui::Label::create(item->m_materialType.name), 2, column);
			// Only one of these will be used but they should not share a column.
			if(item->m_itemType.generic)
				equipmentGrid->addWidget(tgui::Label::create(std::to_string(item->getQuantity())), 3, column);
			else
				equipmentGrid->addWidget(tgui::Label::create(std::to_string(item->m_quality)), 4, column);
			equipmentGrid->addWidget(tgui::Label::create(std::to_string(item->getMass())), 5, column);
			if(!item->m_name.empty())
				equipmentGrid->addWidget(tgui::Label::create(item->m_name), 6, column);
			++column;
		}
		layout->add(equipmentGrid);
		layout->add(uniformUI);
	}
	if(m_actor->getFaction())
	{
		auto uniformUI = tgui::ComboBox::create();
		layout->add(uniformUI);
		for(auto& pair : m_window.getSimulation()->m_hasUniforms.at(*m_window.getFaction()).getAll())
			uniformUI->addItem(pair.first);
		if(actor.m_hasUniform.exists())
			uniformUI->setSelectedItem(actor.m_hasUniform.get().name);
		uniformUI->onItemSelect([&](const tgui::String& uniformName){
			if(!m_actor->getFaction())
				return;
			Uniform& uniform = m_window.getSimulation()->m_hasUniforms.at(*m_actor->getFaction()).at(uniformName.toWideString());
			m_actor->m_hasUniform.set(uniform);
		});
	}
	// Wounds.
	if(!m_actor->m_body.getAllWounds().empty())
	{
		layout->add(tgui::Label::create("wounds"));
		auto woundsGrid = tgui::Grid::create();
		woundsGrid->addWidget(tgui::Label::create("body part"), 1, 1);
		woundsGrid->addWidget(tgui::Label::create("area"), 2, 1);
		woundsGrid->addWidget(tgui::Label::create("depth"), 3, 1);
		woundsGrid->addWidget(tgui::Label::create("bleed"), 4, 1);
		auto woundCount = 1;
		for(Wound* wound : m_actor->m_body.getAllWounds())
		{
			woundsGrid->addWidget(tgui::Label::create(wound->bodyPart.bodyPartType.name), 1, ++woundCount);
			woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.area)), 2, ++woundCount);
			woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.depth)), 3, ++woundCount);
			woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->bleedVolumeRate)), 4, ++woundCount);
		}
		layout->add(woundsGrid);
	}
	// Objective priority button.
	auto showObjectivePriority = tgui::Button::create("priorities");
	layout->add(showObjectivePriority);
	showObjectivePriority->onClick([&]{ m_window.showObjectivePriority(*m_actor); });
	// Close button.
	auto close = tgui::Button::create("close");
	layout->add(close);
	close->onClick([&]{ m_window.showGame(); });
	m_panel->add(layout);
	show();
}
