#include "editActorPanel.h"
#include "displayData.h"
#include "widgets.h"
#include "window.h"
#include "../engine/actors/actors.h"
#include "../engine/items/items.h"
#include "../engine/simulation/hasItems.h"
#include "../engine/body.h"
#include "../engine/definitions/bodyType.h"
#include <TGUI/Widgets/ChildWindow.hpp>
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/EditBox.hpp>
#include <TGUI/Widgets/HorizontalLayout.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <regex>
#include <string>
EditActorView::EditActorView(Window& window) : m_window(window), m_panel(tgui::Panel::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}
void EditActorView::draw(const ActorIndex& actor)
{
	Area& area = *m_window.getArea();
	auto ageUI = tgui::Label::create();
	auto strengthUI = tgui::Label::create();
	auto dextarityUI = tgui::Label::create();
	auto agilityUI = tgui::Label::create();
	std::function<void()> update = [this, &area, actor, ageUI, strengthUI, dextarityUI, agilityUI]{
		Actors& actors = area.getActors();
		ageUI->setText(std::to_string(actors.getAgeInYears(actor).get()));
		strengthUI->setText(std::to_string(actors.getStrength(actor).get()));
		dextarityUI->setText(std::to_string(actors.getDextarity(actor).get()));
		agilityUI->setText(std::to_string(actors.getAgility(actor).get()));
	};
	m_panel->removeAllWidgets();
	m_actor = actor;
	auto layoutGrid = tgui::Grid::create();
	uint8_t gridCount = 0;
	layoutGrid->addWidget(tgui::Label::create("Edit ActorIndex"), ++gridCount, 1);

	auto basicInfoGrid = tgui::Grid::create();
	layoutGrid->addWidget(basicInfoGrid, ++gridCount, 1);
	basicInfoGrid->addWidget(tgui::Label::create("name"), 0, 0);
	auto nameUI = tgui::EditBox::create();
	Actors& actors = area.getActors();
	nameUI->setText(actors.getName(actor));
	nameUI->onTextChange([this, &area, actor](const tgui::String& value){
		Actors& actors = area.getActors();
		actors.setName(actor, value.toWideString());
	});
	basicInfoGrid->addWidget(nameUI, 0, 1);
	basicInfoGrid->addWidget(tgui::Label::create("date of birth"), 0, 2);
	DateTimeUI dateOfBirth(actors.getBirthStep(actor));
	dateOfBirth.m_hours->onValueChange([this, &area, update](float value){
		Actors& actors = area.getActors();
		DateTime old(actors.getBirthStep(m_actor));
		actors.setBirthStep(m_actor, DateTime::toSteps(value, old.day, old.year));
		update();
	});
	dateOfBirth.m_days->onValueChange([this, &area, update](float value){
		Actors& actors = area.getActors();
		DateTime old(actors.getBirthStep(m_actor));
		actors.setBirthStep(m_actor, DateTime::toSteps(old.hour, value, old.year));
		update();
	});
	dateOfBirth.m_years->onValueChange([this, &area, update](float value){
		Actors& actors = area.getActors();
		DateTime old(actors.getBirthStep(m_actor));
		actors.setBirthStep(m_actor, DateTime::toSteps(old.hour, old.day, value));
		update();
	});
	basicInfoGrid->addWidget(dateOfBirth.m_widget, 0, 3);
	basicInfoGrid->addWidget(tgui::Label::create("age"), 0, 4);
	basicInfoGrid->addWidget(ageUI, 0, 5);
	basicInfoGrid->addWidget(tgui::Label::create("percent grown"), 0, 6);
	auto grownUI = tgui::SpinControl::create();
	grownUI->setMaximum(100);
	grownUI->setMinimum(1);
	grownUI->setValue(actors.getPercentGrown(m_actor).get());
	basicInfoGrid->addWidget(grownUI, 0, 7);
	grownUI->onValueChange([this, &area, update](float value){
		Actors& actors = area.getActors();
		actors.grow_setPercent(m_actor, Percent::create(value));
		update();
	});
	basicInfoGrid->addWidget(tgui::Label::create("faction"), 0, 8);
	auto factionUI = widgetUtil::makeFactionSelectUI(*m_window.getSimulation(), L"none");
	const FactionId& actorFactionId = actors.getFaction(m_actor);
	if(actorFactionId.exists())
	{
		const Faction& faction = m_window.getSimulation()->m_hasFactions.getById(actorFactionId);
		factionUI->setSelectedItem(faction.name);
	}
	factionUI->onItemSelect([this, &area, update](const tgui::String factionName){
		Actors& actors = area.getActors();
		const FactionId& faction = m_window.getSimulation()->m_hasFactions.byName(factionName.toWideString());
		actors.setFaction(m_actor, faction);
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
	strengthBonusOrPenaltyUI->setValue(actors.getStrengthBonusOrPenalty(actor).get());
	attributeGrid->addWidget(strengthBonusOrPenaltyUI, 1, 1);
	strengthBonusOrPenaltyUI->onValueChange([this, &area, actor, update](const float value){
		Actors& actors = area.getActors();
		actors.setStrengthBonusOrPenalty(actor, AttributeLevelBonusOrPenalty::create(value));
		update();
	});
	auto strengthMultiplierUI = tgui::SpinControl::create();
	strengthMultiplierUI->setMaximum(1000);
	strengthMultiplierUI->setMinimum(1);
	strengthMultiplierUI->setValue(actors.getStrengthModifier(actor));
	attributeGrid->addWidget(strengthMultiplierUI, 2, 1);
	strengthMultiplierUI->onValueChange([this, &area, actor, update](const float value){
		Actors& actors = area.getActors();
		actors.setStrengthModifier(actor, value);
		update();
	});
	attributeGrid->addWidget(strengthUI, 4, 1);
	// Dextarity
	attributeGrid->addWidget(tgui::Label::create("dextarity"), 0, 2);
	auto dextarityBonusOrPenaltyUI = tgui::SpinControl::create();
	dextarityBonusOrPenaltyUI->setMaximum(1000);
	dextarityBonusOrPenaltyUI->setMinimum(-1000);
	strengthBonusOrPenaltyUI->setValue(actors.getDextarityBonusOrPenalty(actor).get());
	attributeGrid->addWidget(dextarityBonusOrPenaltyUI, 1, 2);
	dextarityBonusOrPenaltyUI->onValueChange([this, &area, actor, update](const float value){
		Actors& actors = area.getActors();
		actors.setDextarityBonusOrPenalty(actor, AttributeLevelBonusOrPenalty::create(value));
		update();
	});
	auto dextarityMultiplierUI = tgui::SpinControl::create();
	dextarityMultiplierUI->setMaximum(1000);
	dextarityMultiplierUI->setMinimum(1);
	dextarityMultiplierUI->setValue(actors.getDextarityModifier(m_actor));
	attributeGrid->addWidget(dextarityMultiplierUI, 2, 2);
	dextarityMultiplierUI->onValueChange([&area, actor, update](const float value){
		Actors& actors = area.getActors();
		actors.setDextarityModifier(actor, value);
		update();
	});
	attributeGrid->addWidget(dextarityUI, 4, 2);
	// Agility
	attributeGrid->addWidget(tgui::Label::create("agility"), 0, 3);
	auto agilityBonusOrPenaltyUI = tgui::SpinControl::create();
	agilityBonusOrPenaltyUI->setMaximum(1000);
	agilityBonusOrPenaltyUI->setMinimum(-1000);
	agilityBonusOrPenaltyUI->setValue(actors.getAgilityBonusOrPenalty(actor).get());
	attributeGrid->addWidget(agilityBonusOrPenaltyUI, 1, 3);
	agilityBonusOrPenaltyUI->onValueChange([&area, actor, update](const float value){
		Actors& actors = area.getActors();
		actors.setAgilityBonusOrPenalty(actor, AttributeLevelBonusOrPenalty::create(value));
		update();
	});
	auto agilityMultiplierUI = tgui::SpinControl::create();
	agilityMultiplierUI->setMaximum(1000);
	agilityMultiplierUI->setMinimum(1);
	agilityMultiplierUI->setValue(actors.getAgilityModifier(actor));
	attributeGrid->addWidget(agilityMultiplierUI, 2, 3);
	agilityMultiplierUI->onValueChange([&area, actor, update](const float value){
		Actors& actors = area.getActors();
		actors.setAgilityModifier(actor, value);
		update();
	});
	attributeGrid->addWidget(agilityUI, 4, 3);
	// Unencombered carry weight.
	attributeGrid->addWidget(tgui::Label::create("unencombered carry weight"), 0, 4);
	std::wstring localizedNumber = displayData::localizeNumber(actors.getUnencomberedCarryMass(actor).get());
	auto unencomberedCarryWeightUI = tgui::Label::create(localizedNumber);
	attributeGrid->addWidget(unencomberedCarryWeightUI, 1, 4);
	// Move speed.
	attributeGrid->addWidget(tgui::Label::create("move speed"), 0, 5);
	auto moveSpeedUI = tgui::Label::create(std::to_string(actors.attributes_getMoveSpeed(actor).get()));
	attributeGrid->addWidget(moveSpeedUI, 1, 5);
	// Base combat score.
	attributeGrid->addWidget(tgui::Label::create("base combat score"), 0, 6);
	localizedNumber = displayData::localizeNumber(actors.attributes_getCombatScore(actor).get());
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
	SkillSet& skillSet = actors.skill_getSet(actor);
	for(auto& [skillType, skill] : skillSet.getSkills())
	{
		skillsGrid->addWidget(tgui::Label::create(SkillType::getName(skillType)), 1, skillsCount);
		auto levelUI = tgui::SpinControl::create();
		levelUI->setValue(skill.m_level.get());
		levelUI->setMinimum(0);
		levelUI->setMaximum(INT32_MAX);
		Skill* skillPointer = &skill;
		levelUI->onValueChange([skillPointer](const float& value){ skillPointer->m_level = SkillLevel::create(value); });
		skillsGrid->addWidget(levelUI, 2, skillsCount);
		auto xpText = std::to_string(skill.m_xp.get()) + "/" + std::to_string(skill.m_xpForNextLevel.get());
		skillsGrid->addWidget(tgui::Label::create(xpText), 3, skillsCount);
		auto remove = tgui::Button::create("remove");
		remove->onClick([this, skillSet, actor, skillType] mutable { skillSet.getSkills().erase(skillType); draw(actor); });
		skillsGrid->addWidget(remove, 4, skillsCount);
		++skillsCount;
	}
	auto createSkill = tgui::ComboBox::create();
	createSkill->setDefaultText("select new skill");
	for(const std::wstring& name : SkillType::getNames())
		createSkill->addItem(name, name);
	layoutGrid->addWidget(createSkill, ++gridCount, 1);
	createSkill->onItemSelect([this, &actors, actor](const tgui::String& value){
		SkillSet& skillSet = actors.skill_getSet(actor);
		const SkillTypeId& skillType = SkillType::byName(value.toWideString());
		skillSet.addXp(skillType, SkillExperiencePoints::create(1));
		draw(m_actor);
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
	EquipmentSet& equipmentSet = actors.equipment_getSet(m_actor);
	Items& items = area.getItems();
	for(const ItemReference& itemReference : equipmentSet.getAll())
	{
		const ItemIndex& item = itemReference.getIndex(items.m_referenceData);
		equipmentGrid->addWidget(tgui::Label::create(MaterialType::getName(items.getMaterialType(item))), 2, count);
		// Only one of these will be used but they should not share a count.
		if(items.isGeneric(item))
			equipmentGrid->addWidget(tgui::Label::create(std::to_string(items.getQuantity(item).get())), 3, count);
		else
			equipmentGrid->addWidget(tgui::Label::create(std::to_string(items.getQuality(item).get())), 4, count);
		equipmentGrid->addWidget(tgui::Label::create(std::to_string(items.getMass(item).get())), 5, count);
		std::wstring name = items.getName(item);
		if(!name.empty())
			equipmentGrid->addWidget(tgui::Label::create(name), 6, count);
		auto destroy = tgui::Button::create("destroy");
		equipmentGrid->addWidget(destroy, 7, count);
		destroy->onClick([this, &area, item, &equipmentSet] mutable {
			equipmentSet.removeEquipment(area, item);
			draw(m_actor);
		});
		++count;
	}
	auto createEquipment = tgui::Button::create("create equipment");
	layoutGrid->addWidget(createEquipment, ++gridCount, 1);
	createEquipment->onClick([this, &area, &equipmentSet]{
		auto childWindow = tgui::ChildWindow::create();
		childWindow->setTitle("create equipment");
		m_panel->add(childWindow);
		auto childLayout = tgui::VerticalLayout::create();
		childWindow->add(childLayout);
		std::function<void(ItemParamaters params)> callback = [this, &area, &equipmentSet](ItemParamaters params){
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			const ItemIndex& newItem = area.getItems().create(params);
			equipmentSet.addEquipment(area, newItem);
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
	for(Wound* wound : actors.body_getWounds(m_actor))
	{
		woundsGrid->addWidget(tgui::Label::create(BodyPartType::getName(wound->bodyPart.bodyPartType)), 1, woundCount);
		woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.area)), 2, woundCount);
		woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->hit.depth)), 3, woundCount);
		woundsGrid->addWidget(tgui::Label::create(std::to_string(wound->bleedVolumeRate)), 4, woundCount);
		++woundCount;
	}
	//TODO: create / remove wound. Amputations.
	if(actors.isSentient(m_actor))
	{
		// Objective priority button.
		auto showObjectivePriority = tgui::Button::create("priorities");
		layoutGrid->addWidget(showObjectivePriority, ++gridCount, 1);
		showObjectivePriority->onClick([&]{ m_window.showObjectivePriority(m_actor); });
		Area& area = *m_window.getArea();
		Actors& actors = area.getActors();
		if(actorFactionId.exists())
		{
			// Uniform
			auto uniformUI = tgui::ComboBox::create();
			layoutGrid->addWidget(uniformUI, ++gridCount, 1);
			uniformUI->onItemSelect([&](const tgui::String& uniformName){
				if(!actorFactionId.exists())
					return;
				Uniform& uniform = m_window.getSimulation()->m_hasUniforms.getForFaction(actorFactionId).byName(uniformName.toWideString());
				actors.uniform_set(m_actor, uniform);
			});
			for(auto& pair : m_window.getSimulation()->m_hasUniforms.getForFaction(actorFactionId).getAll())
				uniformUI->addItem(pair.first);
			if(actors.uniform_exists(m_actor))
				uniformUI->setSelectedItem(actors.uniform_get(m_actor).name);
		}
	}
	// Reset needs button.
	auto resetNeeds = tgui::Button::create("reset needs");
	layoutGrid->addWidget(resetNeeds, ++gridCount, 1);
	resetNeeds->onClick([this, &area]{ area.getActors().resetNeeds(m_actor); });
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
