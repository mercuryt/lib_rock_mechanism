#include "editActorPanel.h"
#include "displayData.h"
#include "widgets.h"
#include "window.h"
#include "../engine/actor.h"
#include "../engine/item.h"
#include "../engine/simulation/hasItems.h"
#include <TGUI/Widgets/ChildWindow.hpp>
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/EditBox.hpp>
#include <TGUI/Widgets/HorizontalLayout.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <regex>
#include <string>
EditActorView::EditActorView(Window& window) : m_window(window), m_panel(tgui::Panel::create()), m_actor(nullptr)
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}
void EditActorView::draw(Actor& actor)
{
	auto ageUI = tgui::Label::create();
	auto strengthUI = tgui::Label::create();
	auto dextarityUI = tgui::Label::create();
	auto agilityUI = tgui::Label::create();
	std::function<void()> update = [&actor, ageUI, strengthUI, dextarityUI, agilityUI]{
		ageUI->setText(std::to_string(actor.getAgeInYears()));
		strengthUI->setText(std::to_string(actor.m_attributes.getStrength()));
		dextarityUI->setText(std::to_string(actor.m_attributes.getDextarity()));
		agilityUI->setText(std::to_string(actor.m_attributes.getAgility()));
	};
	m_panel->removeAllWidgets();
	m_actor = &actor;
	auto layoutGrid = tgui::Grid::create();
	uint8_t gridCount = 0;
	layoutGrid->addWidget(tgui::Label::create("Edit Actor"), ++gridCount, 1);

	auto basicInfoGrid = tgui::Grid::create();
	layoutGrid->addWidget(basicInfoGrid, ++gridCount, 1);
	basicInfoGrid->addWidget(tgui::Label::create("name"), 0, 0);
	auto nameUI = tgui::EditBox::create();
	nameUI->setText(actor.m_name);
	nameUI->onTextChange([&actor](const tgui::String& value){ actor.m_name = value.toWideString(); });
	basicInfoGrid->addWidget(nameUI, 0, 1);
	basicInfoGrid->addWidget(tgui::Label::create("date of birth"), 0, 2);
	DateTimeUI dateOfBirth(actor.getBirthStep());
	dateOfBirth.m_hours->onValueChange([this, update](float value){ 
		DateTime old(m_actor->getBirthStep());
		m_actor->setBirthStep(DateTime::toSteps(value, old.day, old.year));
		update();
	});
	dateOfBirth.m_days->onValueChange([this, update](float value){ 
		DateTime old(m_actor->getBirthStep());
		m_actor->setBirthStep(DateTime::toSteps(old.hour, value, old.year));
		update();
	});
	dateOfBirth.m_years->onValueChange([this, update](float value){ 
		DateTime old(m_actor->getBirthStep());
		m_actor->setBirthStep(DateTime::toSteps(old.hour, old.day, value));
		update();
	});
	basicInfoGrid->addWidget(dateOfBirth.m_widget, 0, 3);
	basicInfoGrid->addWidget(tgui::Label::create("age"), 0, 4);
	basicInfoGrid->addWidget(ageUI, 0, 5);
	basicInfoGrid->addWidget(tgui::Label::create("percent grown"), 0, 6);
	auto grownUI = tgui::SpinControl::create();
	grownUI->setMaximum(100);
	grownUI->setMinimum(1);
	grownUI->setValue(m_actor->m_canGrow.growthPercent());
	basicInfoGrid->addWidget(grownUI, 0, 7);
	grownUI->onValueChange([this, update](float value){
		m_actor->m_canGrow.setGrowthPercent(value);
		update();
	});
	basicInfoGrid->addWidget(tgui::Label::create("faction"), 0, 8);
	auto factionUI = widgetUtil::makeFactionSelectUI(*m_window.getSimulation(), L"none");
	if(m_actor->getFaction())
		factionUI->setSelectedItem(m_actor->getFaction()->name);
	factionUI->onItemSelect([this, update](const tgui::String factionName){
		Faction& faction = m_window.getSimulation()->m_hasFactions.byName(factionName.toWideString());
		m_actor->setFaction(&faction);
	});
	basicInfoGrid->addWidget(factionUI, 0, 9);
	// Attributes.
	layoutGrid->addWidget(tgui::Label::create("attributes"), ++gridCount, 1);
	auto attributeGrid = tgui::Grid::create();
	layoutGrid->addWidget(attributeGrid, ++gridCount, 1);
	attributeGrid->addWidget(tgui::Label::create("+"), 1, 0);
	attributeGrid->addWidget(tgui::Label::create("*"), 2, 0);
	// Strength
	attributeGrid->addWidget(tgui::Label::create("strength"), 0, 1);
	auto strengthBonusOrPenaltyUI = tgui::SpinControl::create();
	strengthBonusOrPenaltyUI->setMaximum(1000);
	strengthBonusOrPenaltyUI->setMinimum(-1000);
	strengthBonusOrPenaltyUI->setValue(actor.m_attributes.getStrengthBonusOrPenalty());
	attributeGrid->addWidget(strengthBonusOrPenaltyUI, 1, 1);
	strengthBonusOrPenaltyUI->onValueChange([&actor, update](const float value){ 
		actor.m_attributes.setStrengthBonusOrPenalty(value); 
		update();
	});
	auto strengthMultiplierUI = tgui::SpinControl::create();
	strengthMultiplierUI->setMaximum(1000);
	strengthMultiplierUI->setMinimum(1);
	strengthMultiplierUI->setValue(actor.m_attributes.getStrengthModifier());
	attributeGrid->addWidget(strengthMultiplierUI, 2, 1);
	strengthMultiplierUI->onValueChange([&actor, update](const float value){ 
		actor.m_attributes.setStrengthModifier(value); 
		update();
	});
	attributeGrid->addWidget(strengthUI, 4, 1);
	// Dextarity
	attributeGrid->addWidget(tgui::Label::create("dextarity"), 0, 2);
	auto dextarityBonusOrPenaltyUI = tgui::SpinControl::create();
	dextarityBonusOrPenaltyUI->setMaximum(1000);
	dextarityBonusOrPenaltyUI->setMinimum(-1000);
	dextarityBonusOrPenaltyUI->setValue(actor.m_attributes.getDextarityBonusOrPenalty());
	attributeGrid->addWidget(dextarityBonusOrPenaltyUI, 1, 2);
	dextarityBonusOrPenaltyUI->onValueChange([&actor, update](const float value){
	       	actor.m_attributes.setDextarityBonusOrPenalty(value); 
		update();
	});
	auto dextarityMultiplierUI = tgui::SpinControl::create();
	dextarityMultiplierUI->setMaximum(1000);
	dextarityMultiplierUI->setMinimum(1);
	dextarityMultiplierUI->setValue(actor.m_attributes.getDextarityModifier());
	attributeGrid->addWidget(dextarityMultiplierUI, 2, 2);
	dextarityMultiplierUI->onValueChange([&actor, update](const float value){ 
		actor.m_attributes.setDextarityModifier(value); 
		update();
	});
	attributeGrid->addWidget(dextarityUI, 4, 2);
	// Agility
	attributeGrid->addWidget(tgui::Label::create("agility"), 0, 3);
	auto agilityBonusOrPenaltyUI = tgui::SpinControl::create();
	agilityBonusOrPenaltyUI->setMaximum(1000);
	agilityBonusOrPenaltyUI->setMinimum(-1000);
	agilityBonusOrPenaltyUI->setValue(actor.m_attributes.getAgilityBonusOrPenalty());
	attributeGrid->addWidget(agilityBonusOrPenaltyUI, 1, 3);
	agilityBonusOrPenaltyUI->onValueChange([&actor, update](const float value){ 
		actor.m_attributes.setAgilityBonusOrPenalty(value); 
		update();
	});
	auto agilityMultiplierUI = tgui::SpinControl::create();
	agilityMultiplierUI->setMaximum(1000);
	agilityMultiplierUI->setMinimum(1);
	agilityMultiplierUI->setValue(actor.m_attributes.getAgilityModifier());
	attributeGrid->addWidget(agilityMultiplierUI, 2, 3);
	agilityMultiplierUI->onValueChange([&actor, update](const float value){ 
		actor.m_attributes.setAgilityModifier(value); 
		update();
	});
	attributeGrid->addWidget(agilityUI, 4, 3);
	// Unencombered carry weight.
	attributeGrid->addWidget(tgui::Label::create("unencombered carry weight"), 0, 4);
	std::string localizedNumber = displayData::localizeNumber(actor.m_attributes.getUnencomberedCarryMass());
	auto unencomberedCarryWeightUI = tgui::Label::create(localizedNumber);
	attributeGrid->addWidget(unencomberedCarryWeightUI, 1, 4);
	// Move speed.
	attributeGrid->addWidget(tgui::Label::create("move speed"), 0, 5);
	auto moveSpeedUI = tgui::Label::create(std::to_string(actor.m_attributes.getMoveSpeed()));
	attributeGrid->addWidget(moveSpeedUI, 1, 5);
	// Base combat score.
	attributeGrid->addWidget(tgui::Label::create("base combat score"), 0, 6);
	localizedNumber = displayData::localizeNumber(actor.m_attributes.getBaseCombatScore());
	auto baseCombatScoreUI = tgui::Label::create(localizedNumber);
	attributeGrid->addWidget(baseCombatScoreUI, 1, 6);
	// Skills.
	layoutGrid->addWidget(tgui::Label::create("skills"), ++gridCount, 1);
	auto skillsGrid = tgui::Grid::create();
	uint8_t skillsCount = 2;
	layoutGrid->addWidget(skillsGrid, ++gridCount, 1);
	skillsGrid->addWidget(tgui::Label::create("name"), 1, 1);
	skillsGrid->addWidget(tgui::Label::create("level"), 2, 1);
	skillsGrid->addWidget(tgui::Label::create("xp"), 3, 1);
	for(auto& [skillType, skill] : m_actor->m_skillSet.m_skills)
	{
		skillsGrid->addWidget(tgui::Label::create(skillType->name), 1, skillsCount);
		auto levelUI = tgui::SpinControl::create();
		levelUI->setValue(skill.m_level);
		levelUI->setMinimum(0);
		levelUI->setMaximum(UINT32_MAX);
		Skill* skillPointer = &skill;
		levelUI->onValueChange([skillPointer](const float& value){ skillPointer->m_level = value; });
		skillsGrid->addWidget(levelUI, 2, skillsCount);
		auto xpText = std::to_string(skill.m_xp) + "/" + std::to_string(skill.m_xpForNextLevel);
		skillsGrid->addWidget(tgui::Label::create(xpText), 3, skillsCount);
		auto remove = tgui::Button::create("remove");
		remove->onClick([this, &actor, skillType]{ m_actor->m_skillSet.m_skills.erase(skillType); draw(actor); });
		skillsGrid->addWidget(remove, 4, skillsCount);
		++skillsCount;
	}
	auto createSkill = tgui::ComboBox::create();
	createSkill->setDefaultText("select new skill");
	for(const SkillType& skillType : skillTypeDataStore)
		createSkill->addItem(skillType.name, skillType.name);
	layoutGrid->addWidget(createSkill, ++gridCount, 1);
	createSkill->onItemSelect([this](const tgui::String& value){
		const SkillType& skillType = SkillType::byName(value.toStdString());
		m_actor->m_skillSet.addXp(skillType, 1u);
		draw(*m_actor);
	});
	// Equipment.
	layoutGrid->addWidget(tgui::Label::create("equipment"), ++gridCount, 1);
	auto equipmentGrid = tgui::Grid::create();
	layoutGrid->addWidget(equipmentGrid, ++gridCount, 1);
	equipmentGrid->addWidget(tgui::Label::create("type"), 1, 1);
	equipmentGrid->addWidget(tgui::Label::create("material"), 2, 1);
	equipmentGrid->addWidget(tgui::Label::create("quantity"), 3, 1);
	equipmentGrid->addWidget(tgui::Label::create("quality"), 4, 1);
	equipmentGrid->addWidget(tgui::Label::create("mass"), 5, 1);
	uint8_t count = 2;
	for(Item* item : m_actor->m_equipmentSet.getAll())
	{
		equipmentGrid->addWidget(tgui::Label::create(item->m_itemType.name), 1, count);
		equipmentGrid->addWidget(tgui::Label::create(item->m_materialType.name), 2, count);
		// Only one of these will be used but they should not share a count.
		if(item->m_itemType.generic)
			equipmentGrid->addWidget(tgui::Label::create(std::to_string(item->getQuantity())), 3, count);
		else
			equipmentGrid->addWidget(tgui::Label::create(std::to_string(item->m_quality)), 4, count);
		equipmentGrid->addWidget(tgui::Label::create(std::to_string(item->getMass())), 5, count);
		if(!item->m_name.empty())
			equipmentGrid->addWidget(tgui::Label::create(item->m_name), 6, count);
		auto destroy = tgui::Button::create("destroy");
		equipmentGrid->addWidget(destroy, 7, count);
		destroy->onClick([this, item]{ m_actor->m_equipmentSet.removeEquipment(*item); draw(*m_actor); });
		++count;
	}
	auto createEquipment = tgui::Button::create("create equipment");
	layoutGrid->addWidget(createEquipment, ++gridCount, 1);
	createEquipment->onClick([this]{
		auto childWindow = tgui::ChildWindow::create();
		childWindow->setTitle("create equipment");
		m_panel->add(childWindow);
		auto childLayout = tgui::VerticalLayout::create();
		childWindow->add(childLayout);
		std::function<void(ItemParamaters params)> callback = [this](ItemParamaters params){
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			Item& newItem = m_window.getSimulation()->m_hasItems->createItem(params);
			m_actor->m_equipmentSet.addEquipment(newItem);
			hide();
		};
		auto [itemTypeUI, materialTypeUI, quantityOrQualityLabel, quantityOrQualityUI, wearLabel, wearUI, confirmUI] = widgetUtil::makeCreateItemUI(callback);
		auto itemTypeLabel = tgui::Label::create("item type");
		itemTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		childWindow->add(itemTypeLabel);
		childWindow->add(itemTypeUI);
		auto materialTypeLabel = tgui::Label::create("material type");
		materialTypeLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		childWindow->add(materialTypeLabel);
		childWindow->add(materialTypeUI);

		quantityOrQualityLabel->cast<tgui::Label>()->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		childWindow->add(quantityOrQualityLabel);
		childWindow->add(quantityOrQualityUI);
		wearLabel->cast<tgui::Label>()->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
		childWindow->add(wearLabel);
		childWindow->add(wearUI);
		childWindow->add(confirmUI);
	});
	// Wounds.
	layoutGrid->addWidget(tgui::Label::create("wounds"), ++gridCount, 1);
	auto woundsGrid = tgui::Grid::create();
	layoutGrid->addWidget(woundsGrid, ++gridCount, 1);
	woundsGrid->addWidget(tgui::Label::create("body part"), 1, 1);
	woundsGrid->addWidget(tgui::Label::create("area"), 2, 1);
	woundsGrid->addWidget(tgui::Label::create("depth"), 3, 1);
	woundsGrid->addWidget(tgui::Label::create("bleed"), 4, 1);
	uint8_t woundCount = 2;
	for(Wound* wound : m_actor->m_body.getAllWounds())
	{
		woundsGrid->addWidget(tgui::Label::create(wound->bodyPart.bodyPartType.name), 1, woundCount);
		woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.area)), 2, woundCount);
		woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.depth)), 3, woundCount);
		woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->bleedVolumeRate)), 4, woundCount);
		++woundCount;
	}
	//TODO: create / remove wound. Amputations.
	if(m_actor->isSentient())
	{
		// Objective priority button.
		auto showObjectivePriority = tgui::Button::create("priorities");
		layoutGrid->addWidget(showObjectivePriority, ++gridCount, 1);
		showObjectivePriority->onClick([&]{ m_window.showObjectivePriority(*m_actor); });
		if(m_actor->getFaction())
		{
			// Uniform
			auto uniformUI = tgui::ComboBox::create();
			layoutGrid->addWidget(uniformUI, ++gridCount, 1);
			uniformUI->onItemSelect([&](const tgui::String& uniformName){
				if(!m_actor->getFaction())
					return;
				Uniform& uniform = m_window.getSimulation()->m_hasUniforms.at(*m_actor->getFaction()).at(uniformName.toWideString());
				m_actor->m_hasUniform.set(uniform);
			});
			for(auto& pair : m_window.getSimulation()->m_hasUniforms.at(*m_window.getFaction()).getAll())
				uniformUI->addItem(pair.first);
			if(actor.m_hasUniform.exists())
				uniformUI->setSelectedItem(actor.m_hasUniform.get().name);
		}
	}
	// Reset needs button.
	auto resetNeeds = tgui::Button::create("reset needs");
	layoutGrid->addWidget(resetNeeds, ++gridCount, 1);
	resetNeeds->onClick([this]{ m_actor->resetNeeds(); });
	// Close button.
	auto close = tgui::Button::create("close");
	layoutGrid->addWidget(close, ++gridCount, 1);
	close->onClick([&]{ m_window.showGame(); });
	m_panel->add(layoutGrid);
	layoutGrid->setHeight("100%");
	layoutGrid->setOrigin(0.5, 0);
	layoutGrid->setPosition("50%", 0);
	m_panel->setVisible(true);
	update();
}
