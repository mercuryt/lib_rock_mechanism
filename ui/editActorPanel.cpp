#include "editActorPanel.h"
#include "widgets.h"
#include "window.h"
#include "../engine/actor.h"
#include <TGUI/Widgets/ChildWindow.hpp>
#include <TGUI/Widgets/ComboBox.hpp>
#include <TGUI/Widgets/EditBox.hpp>
#include <TGUI/Widgets/HorizontalLayout.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
EditActorView::EditActorView(Window& window) : m_window(window), m_panel(tgui::Panel::create()), m_actor(nullptr)
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}
void EditActorView::draw(Actor& actor)
{
	m_panel->removeAllWidgets();
	m_actor = &actor;
	auto layoutGrid = tgui::Grid::create();
	uint8_t gridCount = 0;
	layoutGrid->addWidget(tgui::Label::create("Edit Actor"), ++gridCount, 1);

	auto basicInfoGrid = tgui::Grid::create();
	layoutGrid->addWidget(basicInfoGrid, ++gridCount, 1);
	basicInfoGrid->addWidget(tgui::Label::create("name"), 1, 1);
	auto nameUI = tgui::EditBox::create();
	nameUI->setText(actor.m_name);
	nameUI->onTextChange([&actor](const tgui::String& value){ actor.m_name = value.toWideString(); });
	basicInfoGrid->addWidget(nameUI, 1, 2);
	basicInfoGrid->addWidget(tgui::Label::create("date of birth"), 2, 1);
	DateTimeUI dateOfBirth(actor.m_birthDate);
	basicInfoGrid->addWidget(dateOfBirth.m_widget, 2, 2);

	// Attributes.
	layoutGrid->addWidget(tgui::Label::create("attributes"), ++gridCount, 1);
	auto attributeGrid = tgui::Grid::create();
	layoutGrid->addWidget(attributeGrid, ++gridCount, 1);
	// Strength
	attributeGrid->addWidget(tgui::Label::create("strength"), 1, 1);
	auto strengthModifierUI = tgui::SpinControl::create();
	strengthModifierUI->setMaximum(INT32_MAX);
	strengthModifierUI->setMinimum(-(float)INT32_MAX);
	strengthModifierUI->setValue(actor.m_attributes.getStrength());
	attributeGrid->addWidget(strengthModifierUI, 2, 1);
	strengthModifierUI->onValueChange([&actor](const float value){ actor.m_attributes.setStrengthBonusOrPenalty(value); });
	// Dextarity
	attributeGrid->addWidget(tgui::Label::create("dextarity"), 1, 2);
	auto dextarityModifierUI = tgui::SpinControl::create();
	dextarityModifierUI->setMaximum(UINT32_MAX);
	dextarityModifierUI->setMinimum(1);
	dextarityModifierUI->setValue(actor.m_attributes.getDextarity());
	attributeGrid->addWidget(dextarityModifierUI, 2, 2);
	dextarityModifierUI->onValueChange([&actor](const float value){ actor.m_attributes.setDextarityBonusOrPenalty(value); });
	// Agility
	attributeGrid->addWidget(tgui::Label::create("agility"), 1, 3);
	auto agilityModifierUI = tgui::SpinControl::create();
	agilityModifierUI->setMaximum(UINT32_MAX);
	agilityModifierUI->setMinimum(1);
	agilityModifierUI->setValue(actor.m_attributes.getAgility());
	attributeGrid->addWidget(agilityModifierUI, 2, 3);
	agilityModifierUI->onValueChange([&actor](const float value){ actor.m_attributes.setAgilityBonusOrPenalty(value); });

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
	for(const SkillType& skillType : SkillType::data)
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
		auto itemTypeUI = widgetUtil::makeItemTypeSelectUI();
		childLayout->add(itemTypeUI);
		auto materialTypeUI = widgetUtil::makeMaterialSelectUI();
		childLayout->add(materialTypeUI);
		childLayout->add(tgui::Label::create("quality"));
		auto qualityUI = tgui::SpinControl::create();
		qualityUI->setMinimum(0);
		qualityUI->setMaximum(UINT32_MAX);
		childLayout->add(qualityUI);
		childLayout->add(tgui::Label::create("wear"));
		auto wearUI = tgui::SpinControl::create();
		wearUI->setMinimum(0);
		wearUI->setMaximum(100);
		childLayout->add(wearUI);
		childLayout->add(tgui::Label::create("quantity"));
		auto quantityUI = tgui::SpinControl::create();
		quantityUI->setMinimum(1);
		quantityUI->setMaximum(UINT32_MAX);
		childLayout->add(quantityUI);
		auto confirm = tgui::Button::create("confirm");
		childLayout->add(confirm);
		confirm->onClick([this, itemTypeUI, materialTypeUI, quantityUI, qualityUI, wearUI, childWindow]{
			const ItemType& itemType = ItemType::byName(itemTypeUI->getSelectedItemId().toStdString());
			const MaterialType& materialType = MaterialType::byName(materialTypeUI->getSelectedItemId().toStdString());
			if(itemType.generic)
			{
				uint32_t quantity = quantityUI->getValue();
				Item& item = m_window.getSimulation()->createItemGeneric(itemType, materialType, quantity);
				m_actor->m_equipmentSet.addEquipment(item);
			}
			else
			{
				uint32_t quality = qualityUI->getValue();
				uint32_t wear = wearUI->getValue();
				Item& item = m_window.getSimulation()->createItemNongeneric(itemType, materialType, quality, wear);
				m_actor->m_equipmentSet.addEquipment(item);
			}
			childWindow->close();
			draw(*m_actor);
		});
	});
	// Uniform
	auto uniformUI = tgui::ComboBox::create();
	layoutGrid->addWidget(uniformUI, ++gridCount, 1);
	uniformUI->onItemSelect([&](const tgui::String& uniformName){
		if(!m_actor->getFaction())
			return;
		Uniform& uniform = m_window.getSimulation()->m_hasUniforms.at(*m_actor->getFaction()).at(uniformName.toWideString());
		m_actor->m_hasUniform.set(uniform);
	});
	if(m_actor->getFaction())
	{
		for(auto& pair : m_window.getSimulation()->m_hasUniforms.at(*m_window.getFaction()).getAll())
			uniformUI->addItem(pair.first);
		if(actor.m_hasUniform.exists())
			uniformUI->setSelectedItem(actor.m_hasUniform.get().name);
	}
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
	// Objective priority button.
	auto showObjectivePriority = tgui::Button::create("priorities");
	layoutGrid->addWidget(showObjectivePriority, ++gridCount, 1);
	showObjectivePriority->onClick([&]{ m_window.showObjectivePriority(*m_actor); });
	// Close button.
	auto close = tgui::Button::create("close");
	layoutGrid->addWidget(close, ++gridCount, 1);
	close->onClick([&]{ m_window.showGame(); });
	m_panel->add(layoutGrid);
	layoutGrid->setHeight("100%");
	layoutGrid->setOrigin(0.5, 0);
	layoutGrid->setPosition("50%", 0);
	m_panel->setVisible(true);
}
