#include "screens.h"
#include "../window.h"
#include "../../engine/area/area.h"
#include "../../engine/actors/actors.h"
#include "../../engine/actors/skill.h"
#include "../../engine/items/items.h"
#include "../../engine/body.h"
#include "../../engine/uniform.h"
#include "../../engine/equipment.h"
#include "../../engine/definitions/bodyType.h"

void screens::actorDetails(Window& window, const ActorReference actorRef)
{
	// Indices are only valid at the current point in time.
	assert(window.m_paused);
	Actors& actors = window.m_area->getActors();
	Items& items = window.m_area->getItems();
	const ActorIndex actor = actorRef.getIndex(actors.m_referenceData);
	begin(window, "Actor Details");
	ImGuiText(("name: " + actors.getName(actor)));
	ImGuiText(("age : " + std::to_string(actors.getAgeInYears(actor).get())));
	// Attributes.
	ImGuiText("attributes");
	ImGui::BeginTable("attributes", 2);
	ImGuiText("strength");
	ImGui::TableNextColumn();
	ImGuiText(actors.getStrength(actor).toString());
	ImGui::TableNextRow();
	ImGuiText("dextarity");
	ImGui::TableNextColumn();
	ImGuiText(actors.getDextarity(actor).toString());
	ImGui::TableNextRow();
	ImGuiText("agility");
	ImGui::TableNextColumn();
	ImGuiText(actors.getAgility(actor).toString());
	ImGui::TableNextRow();
	ImGuiText("unencombered carry mass");
	ImGui::TableNextColumn();
	ImGuiText(actors.getUnencomberedCarryMass(actor).toString());
	ImGui::TableNextRow();
	ImGuiText("move speed");
	ImGui::TableNextColumn();
	ImGuiText(actors.move_getSpeed(actor).toString());
	ImGui::TableNextRow();
	ImGuiText("base combat score");
	ImGui::TableNextColumn();
	ImGuiText(actors.combat_getCombatScore(actor).toString());
	ImGui::EndTable();
	// Skills.
	const SkillSet& skillSet = actors.skill_getSet(actor);
	if(!skillSet.getSkills().empty())
	{
		ImGuiText("skills");
		ImGui::BeginTable("skills", 3);
		ImGuiText("name");
		ImGui::TableNextColumn();
		ImGuiText("level");
		ImGui::TableNextColumn();
		ImGuiText("xp");
		ImGui::TableNextRow();
		for(auto& [skillType, skill] : skillSet.getSkills())
		{
			ImGuiText(SkillType::getName(skillType));
			ImGui::TableNextColumn();
			ImGuiText(skill.m_level.toString());
			ImGui::TableNextColumn();
			auto xpText = (skill.m_xp.toString() + "/" + skill.m_xpForNextLevel.toString());
			ImGuiText(xpText);
			ImGui::TableNextRow();
		}
		ImGui::EndTable();
	}
	// Equipment.
	const EquipmentSet& equipmentSet = actors.equipment_getSet(actor);
	if(!equipmentSet.getAll().empty())
	{
		ImGui::BeginTable("equipment", 5);
		ImGuiText("type");
		ImGui::TableNextColumn();
		ImGuiText("material");
		ImGui::TableNextColumn();
		ImGuiText("quantity");
		ImGui::TableNextColumn();
		ImGuiText("quality");
		ImGui::TableNextColumn();
		ImGuiText("mass");
		ImGui::TableNextColumn();
		ImGuiText("name");
		ImGui::TableNextRow();
		for(const ItemReference& itemReference : equipmentSet.getAll())
		{
			const ItemIndex& item = itemReference.getIndex(items.m_referenceData);
			ImGuiText(ItemType::getName(items.getItemType(item)));
			ImGui::TableNextColumn();
			ImGuiText(MaterialType::getName(items.getMaterialType(item)));
			ImGui::TableNextColumn();
			if(items.isGeneric(item))
			{
				ImGuiText(items.getQuantity(item).toString());
				ImGui::TableNextColumn();
				ImGuiText(" ");
			}
			else
			{
				ImGuiText(" ");
				ImGui::TableNextColumn();
				ImGuiText(items.getQuality(item).toString());
			}
			ImGui::TableNextColumn();
			ImGuiText(items.getMass(item).toString());
			ImGui::TableNextColumn();
			ImGuiText(items.getName(item));
			ImGui::TableNextRow();
		}
		ImGui::EndTable();
	}
	// Wounds.
	const auto& wounds = actors.body_getWounds(actor);
	if(!wounds.empty())
	{
		ImGuiText("wounds");
		ImGui::BeginTable("wonunds", 4);
		ImGuiText("body part");
		ImGui::TableNextColumn();
		ImGuiText("area");
		ImGui::TableNextColumn();
		ImGuiText("depth");
		ImGui::TableNextColumn();
		ImGuiText("bleed");
		ImGui::TableNextRow();
		for(Wound* wound : wounds)
		{
			ImGuiText(BodyPartType::getName(wound->bodyPart.bodyPartType));
			ImGui::TableNextColumn();
			ImGuiText(std::to_string(wound->hit.area));
			ImGui::TableNextColumn();
			ImGuiText(std::to_string(wound->hit.depth));
			ImGui::TableNextColumn();
			ImGuiText(std::to_string(wound->bleedVolumeRate));
			ImGui::TableNextRow();
		}
		ImGui::EndTable();
	}
	const FactionId& actorFaction = actors.getFaction(actor);
	if(actorFaction.exists())
	{

		std::string equipedName = actors.uniform_exists(actor) ? actors.uniform_get(actor).name : "";
		ImGuiBeginCombo("uniform", equipedName);
		for(auto& [name, uniform] : window.m_simulation->m_hasUniforms.getForFaction(window.m_faction).getAll())
		{
			bool isSelected = equipedName == name;
			if(ImGuiSelectable(name, isSelected))
				actors.uniform_set(actor, uniform);
			if(isSelected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
		// Objective priority button.
		if(window.m_faction.exists() && window.m_faction == actorFaction)
			if(ImGui::Button("Priorities"))
				window.showObjectivePriorities(actors.getReference(actor));

	}
	// Close button.
	if(ImGui::Button("Close"))
		window.showGame();
	end();
}