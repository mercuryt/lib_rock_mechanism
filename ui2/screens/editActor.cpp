
#include "screens.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/actors/actors.h"
#include "../../engine/objective.h"
#include "../../engine/items/items.h"
#include "../../engine/area/area.h"
#include "../../engine/definitions/bodyType.h"

void screens::editActor(Window& window, const ActorReference actorRef)
{
	assert(window.m_editMode);
	window.m_paused = true;
	Area& area = *window.m_area;
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	const ActorIndex actor = actorRef.getIndex(actors.m_referenceData);
	std::string name = actors.getName(actor);
	DateTime birthDate = DateTime(actors.getBirthStep(actor));
	int percentGrown = actors.getPercentGrown(actor).get();
	float strengthModifier = actors.getStrengthModifier(actor);
	int strengthBonus = actors.getStrengthBonusOrPenalty(actor).get();
	int agilityBonus = actors.getAgilityBonusOrPenalty(actor).get();
	float agilityModifier = actors.getAgilityModifier(actor);
	int dextarityBonus = actors.getDextarityBonusOrPenalty(actor).get();
	float dextarityModifier = actors.getDextarityModifier(actor);
	SmallMap<SkillTypeId, int> skillLevels;
	auto onBirthDateChanged = [&]{ actors.setBirthStep(actor, birthDate.toSteps()); };
	FactionId faction;
	begin(window, "Edit Actor");
	// Basic info.
	if(ImGui::InputText("name", &name))
		actors.setName(actor, name);
	ImGuiText(("age in years" + actors.getAgeInYears(actor).toString()));
	if(ImGui::InputInt("birth Year", &birthDate.year))
		onBirthDateChanged();
	ImGui::SameLine();
	if(ImGui::InputInt("birth Day Of Year", &birthDate.day))
		onBirthDateChanged();
	ImGui::SameLine();
	if(ImGui::InputInt("birth Hour", &birthDate.hour))
		onBirthDateChanged();
	if(ImGui::InputInt("percentGrown", &percentGrown))
	{
		percentGrown = std::clamp(percentGrown, 1, 100);
		actors.grow_setPercent(actor, {percentGrown});
	}
	if(widgets::faction(&faction, *window.m_simulation))
		actors.setFaction(actor, faction);
	// Attributes.
	ImGui::BeginTable("attributes", 4);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("value");
	ImGui::TableNextColumn();
	ImGuiText("bonus");
	ImGui::TableNextColumn();
	ImGuiText("modifier");
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("strength");
	ImGui::TableNextColumn();
	ImGuiText(actors.getStrength(actor).toString());
	ImGui::TableNextColumn();
	if(ImGui::InputInt("##strengthBonus", &strengthBonus))
		actors.setStrengthBonusOrPenalty(actor, {strengthBonus});
	ImGui::TableNextColumn();
	if(ImGui::InputFloat("##strengthModifier", &strengthModifier))
		actors.setStrengthModifier(actor, strengthModifier);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("agility");
	ImGui::TableNextColumn();
	ImGuiText(actors.getAgility(actor).toString());
	ImGui::TableNextColumn();
	if(ImGui::InputInt("##agilityBonus", &agilityBonus))
		actors.setAgilityBonusOrPenalty(actor, {agilityBonus});
	ImGui::TableNextColumn();
	if(ImGui::InputFloat("##agilityModifier", &agilityModifier))
		actors.setAgilityModifier(actor, agilityModifier);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("dextarity");
	ImGui::TableNextColumn();
	ImGuiText(actors.getDextarity(actor).toString());
	ImGui::TableNextColumn();
	if(ImGui::InputInt("##dextarityBonus", &dextarityBonus))
		actors.setDextarityBonusOrPenalty(actor, {dextarityBonus});
	ImGui::TableNextColumn();
	if(ImGui::InputFloat("##dextarityModifier", &dextarityModifier))
		actors.setDextarityModifier(actor, dextarityModifier);
	ImGui::EndTable();
	ImGui::BeginTable("derivedAttributes", 2);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("unencombered carry weight");
	ImGui::TableNextColumn();
	ImGuiText(actors.getUnencomberedCarryMass(actor).toString());
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("move speed");
	ImGui::TableNextColumn();
	ImGuiText(actors.attributes_getMoveSpeed(actor).toString());
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("base combat score");
	ImGui::TableNextColumn();
	ImGuiText(actors.attributes_getCombatScore(actor).toString());
	ImGui::EndTable();
	// Skills.
	ImGui::BeginTable("skills", 4);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("name");
	ImGui::TableNextColumn();
	ImGuiText("level");
	ImGui::TableNextColumn();
	ImGuiText("xp");
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	SkillSet& skillSet = actors.skill_getSet(actor);
	for(auto& [skillType, skill] : skillSet.getSkills())
	{
		skillLevels[skillType] = skill.m_level.get();
		std::string skillName = SkillType::getName(skillType);
		ImGuiText(skillName);
		ImGui::TableNextColumn();
		ImGui::InputInt(("##level" + skillName).c_str(), &skillLevels[skillType]);
		ImGui::TableNextColumn();
		auto xpText = std::to_string(skill.m_xp.get()) + "/" + std::to_string(skill.m_xpForNextLevel.get());
		ImGuiText(xpText);
		ImGui::TableNextColumn();
		if(ImGuiButton("x##remove" + skillName))
			skillSet.getSkills().erase(skillType);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
	}
	ImGui::EndTable();
	if(ImGui::BeginCombo("add skill", "add skill"))
	{
		for(const auto& skillName : SkillType::getNames())
		{
			if(ImGuiSelectable(skillName, false))
			{
				SkillSet& skillSet2 = actors.skill_getSet(actor);
				const SkillTypeId& skillType = SkillType::byName(skillName);
				skillSet2.addXp(skillType, SkillExperiencePoints::create(1));
			}
		}
		ImGui::EndCombo();
	}
	// List equipment.
	ImGui::BeginTable("equipment", 6);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("type");
	ImGui::TableNextColumn();
	ImGuiText("material");
	ImGui::TableNextColumn();
	ImGuiText("quantity");
	ImGui::TableNextColumn();
	ImGuiText("quality");
	ImGui::TableNextColumn();
	ImGuiText("mass");
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	EquipmentSet& equipmentSet = actors.equipment_getSet(actor);
	for(const ItemReference& itemReference : equipmentSet.getAll())
	{
		const ItemIndex item = itemReference.getIndex(items.m_referenceData);
		ImGuiText(ItemType::getName(items.getItemType(item)));
		ImGui::TableNextColumn();
		ImGuiText(MaterialType::getName(items.getMaterialType(item)));
		ImGui::TableNextColumn();
		if(items.isGeneric(item))
			ImGuiText(items.getQuantity(item).toString());
		ImGui::TableNextColumn();
		if(!items.isGeneric(item))
			ImGuiText(items.getQuality(item).toString());
		ImGui::TableNextColumn();
		ImGuiText(items.getMass(item).toString());
		ImGui::TableNextColumn();
		if(ImGuiButton("x##" + item.toString()))
			equipmentSet.removeEquipment(area, item);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
	}
	ImGui::EndTable();
	// New Equipment
	// This is nearly identical to the code in the item create segment of the context menu.
	ControllsState& state = window.m_gameOverlay.m_controllsState;
	widgets::itemType(&state.itemType);
	widgets::materialType(&state.materialType);
	bool isGeneric = ItemType::getIsGeneric(state.itemType);
	if(isGeneric)
		ImGui::InputInt("quantity", &state.quantity.getReference());
	else
	{
		ImGui::InputInt("quality", &state.quality.getReference());
		ImGui::InputInt("wear", &state.wear.getReference());
	}
	if(ImGui::Button("add equipment"))
	{
		ItemParamaters params{
			.itemType=state.itemType,
			.materialType=state.materialType,
			.faction=window.m_faction,
			.location=state.clickedOnPoint,
			.facing=state.facing,
		};
		if(ItemType::getIsGeneric(state.itemType))
			params.quantity = state.quantity;
		else
		{
			params.quality = state.quality;
			params.percentWear = state.wear;
			params.installed = state.installed;
			params.name = state.name;
		}
		ItemIndex newItem = window.m_area->getItems().create(params);
		equipmentSet.addEquipment(area, newItem);
	}
	// Wounds.
	ImGui::BeginTable("wounds", 5);
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGuiText("body part");
	ImGui::TableNextColumn();
	ImGuiText("area");
	ImGui::TableNextColumn();
	ImGuiText("depth");
	ImGui::TableNextColumn();
	ImGuiText("bleed");
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	for(Wound* wound : actors.body_getWounds(actor))
	{
		ImGuiText(BodyPartType::getName(wound->bodyPart.bodyPartType));
		ImGui::TableNextColumn();
		ImGuiText(std::to_string(wound->hit.area));
		ImGui::TableNextColumn();
		ImGuiText(std::to_string(wound->hit.depth));
		ImGui::TableNextColumn();
		ImGuiText(std::to_string(wound->bleedVolumeRate));
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
	}
	ImGui::EndTable();
	if(ImGui::Button("reset needs"))
		area.getActors().resetNeeds(actor);
	if(ImGui::Button("close"))
		window.showGame();
	end();
}