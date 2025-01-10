#include "actorPanel.h"
#include "displayData.h"
#include "window.h"
#include "../engine/actors/actors.h"
#include "../engine/items/items.h"
#include "../engine/bodyType.h"
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>
#include <string>
ActorView::ActorView(Window& window) : m_window(window), m_panel(tgui::ScrollablePanel::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}
void ActorView::draw(const ActorIndex& actor)
{
	Actors& actors = m_window.getArea()->getActors();
	Items& items = m_window.getArea()->getItems();
	m_actor = actor;
	m_panel->removeAllWidgets();
	auto layout = tgui::VerticalLayout::create();
	layout->setPosition("5%", "5%");
	layout->setSize("90%", "90%");
	layout->add(tgui::Label::create("Actor Details"));
	layout->add(tgui::Label::create(L"name: " + actors.getName(actor)));
	layout->add(tgui::Label::create("age : " + std::to_string(actors.getAgeInYears(actor).get())));
	// Attributes.
	layout->add(tgui::Label::create("attributes"));
	auto attributesGrid = tgui::Grid::create();
	attributesGrid->addWidget(tgui::Label::create("strength"), 1, 1);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(actors.getStrength(m_actor).get())), 2, 1);
	attributesGrid->addWidget(tgui::Label::create("dextarity"), 1, 2);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(actors.getDextarity(m_actor).get())), 2, 2);
	attributesGrid->addWidget(tgui::Label::create("agility"), 1, 3);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(actors.getAgility(m_actor).get())), 2, 3);
	attributesGrid->addWidget(tgui::Label::create("unencombered carry mass"), 1, 4);
	attributesGrid->addWidget(tgui::Label::create(displayData::localizeNumber(actors.getUnencomberedCarryMass(m_actor).get())), 2, 4);
	attributesGrid->addWidget(tgui::Label::create("move speed"), 1, 5);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(actors.move_getSpeed(m_actor).get())), 2, 5);
	attributesGrid->addWidget(tgui::Label::create("base combat score"), 1, 6);
	attributesGrid->addWidget(tgui::Label::create(std::to_string(actors.combat_getCombatScore(m_actor).get())), 2, 6);
	layout->add(attributesGrid);
	// Skills.
	const SkillSet& skillSet = actors.skill_getSet(m_actor);
	if(!skillSet.getSkills().empty())
	{
		layout->add(tgui::Label::create("skills"));
		auto skillsGrid = tgui::Grid::create();
		uint32_t skillColumn = 2;
		skillsGrid->addWidget(tgui::Label::create("name"), 1, 1);
		skillsGrid->addWidget(tgui::Label::create("level"), 2, 1);
		skillsGrid->addWidget(tgui::Label::create("xp"), 3, 1);
		for(auto& [skillType, skill] : skillSet.getSkills())
		{
			skillsGrid->addWidget(tgui::Label::create(SkillType::getName(skillType)), 1, skillColumn);
			skillsGrid->addWidget(tgui::Label::create(std::to_string(skill.m_level.get())), 2, skillColumn);
			auto xpText = std::to_string(skill.m_xp.get()) + "/" + std::to_string(skill.m_xpForNextLevel.get());
			skillsGrid->addWidget(tgui::Label::create(xpText), 3, skillColumn);
			++skillColumn;
		}
		layout->add(skillsGrid);
	}
	// Equipment.
	const EquipmentSet& equipmentSet = actors.equipment_getSet(m_actor);
	if(!equipmentSet.getAll().empty())
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
		for(const ItemReference& itemReference : equipmentSet.getAll())
		{
			const ItemIndex& item = itemReference.getIndex(items.m_referenceData);
			equipmentGrid->addWidget(tgui::Label::create(ItemType::getName(items.getItemType(item))), 1, column);
			equipmentGrid->addWidget(tgui::Label::create(MaterialType::getName(items.getMaterialType(item))), 2, column);
			// Only one of these will be used but they should not share a column.
			if(items.isGeneric(item))
				equipmentGrid->addWidget(tgui::Label::create(std::to_string(items.getQuantity(item).get())), 3, column);
			else
				equipmentGrid->addWidget(tgui::Label::create(std::to_string(items.getQuality(item).get())), 4, column);
			equipmentGrid->addWidget(tgui::Label::create(std::to_string(items.getMass(item).get())), 5, column);
			if(!items.getName(item).empty())
				equipmentGrid->addWidget(tgui::Label::create(items.getName(item)), 6, column);
			++column;
		}
		layout->add(equipmentGrid);
		layout->add(uniformUI);
	}
	const FactionId& actorFaction = actors.getFaction(m_actor);
	if(actorFaction.exists())
	{
		auto uniformUI = tgui::ComboBox::create();
		layout->add(uniformUI);
		for(auto& pair : m_window.getSimulation()->m_hasUniforms.getForFaction(m_window.getFaction()).getAll())
			uniformUI->addItem(pair.first);
		const Uniform* actorUniform;
		if(actorUniform != nullptr)
			uniformUI->setSelectedItem(actorUniform->name);
		uniformUI->onItemSelect([&](const tgui::String& uniformName){
			const FactionId& actorFaction = actors.getFaction(m_actor);
			if(actorFaction.empty())
				return;
			Uniform& uniform = m_window.getSimulation()->m_hasUniforms.getForFaction(actorFaction).getAll()[uniformName.toWideString()];
			actors.uniform_set(m_actor, uniform);
		});
	}
	// Wounds.
	const auto& wounds = actors.body_getWounds(m_actor);
	if(!wounds.empty())
	{
		layout->add(tgui::Label::create("wounds"));
		auto woundsGrid = tgui::Grid::create();
		woundsGrid->addWidget(tgui::Label::create("body part"), 1, 1);
		woundsGrid->addWidget(tgui::Label::create("area"), 2, 1);
		woundsGrid->addWidget(tgui::Label::create("depth"), 3, 1);
		woundsGrid->addWidget(tgui::Label::create("bleed"), 4, 1);
		auto woundCount = 1;
		for(Wound* wound : wounds)
		{
			woundsGrid->addWidget(tgui::Label::create(BodyPartType::getName(wound->bodyPart.bodyPartType)), 1, ++woundCount);
			woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.area)), 2, ++woundCount);
			woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.depth)), 3, ++woundCount);
			woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->bleedVolumeRate)), 4, ++woundCount);
		}
		layout->add(woundsGrid);
	}
	// Objective priority button.
	auto showObjectivePriority = tgui::Button::create("priorities");
	layout->add(showObjectivePriority);
	showObjectivePriority->onClick([&]{ m_window.showObjectivePriority(m_actor); });
	// Close button.
	auto close = tgui::Button::create("close");
	layout->add(close);
	close->onClick([&]{ m_window.showGame(); });
	m_panel->add(layout);
	show();
}
