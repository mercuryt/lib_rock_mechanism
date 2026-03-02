#include "screens.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/area/area.h"
std::vector<std::tuple<CraftJobTypeId, MaterialTypeId, Quantity, CraftJob*>> collate(Window& window)
{
	std::vector<std::tuple<CraftJobTypeId, MaterialTypeId, Quantity, CraftJob*>> output;
	for(CraftJob& craftJob : window.m_area->m_hasCraftingLocationsAndJobs.getForFaction(window.m_faction).getAllJobs())
	{
		auto iter = std::ranges::find_if(output, [&](const auto& tuple){
			auto& [craftJobType, materialType, quantity, otherCraftJob] = tuple;
			return craftJobType == craftJob.craftJobType && materialType == craftJob.materialType;
		});
		if(iter == output.end())
			output.emplace_back(craftJob.craftJobType, craftJob.materialType, Quantity::create(1u), &craftJob);
		else
			++std::get<2>(*iter);
	}
	return output;
}
void screens::production(Window& window)
{
	assert(window.m_faction.exists());
	begin(window, "Production Orders");
	// Create.
	CraftJobTypeId craftJobType;
	MaterialTypeId materialType;
	int quantity = 1;
	int minimumSkillLevel = 0;
	auto materialTypeCondition = [&](const MaterialTypeId candidate) {
		return MaterialType::getMaterialTypeCategory(candidate) == CraftJobType::getMaterialTypeCategory(craftJobType);
	};
	widgets::craftJobType(&craftJobType);
	widgets::materialType(&materialType, materialTypeCondition);
	ImGui::InputInt("quantity", &quantity);
	quantity = std::clamp(quantity, 1, Quantity::max().get());
	ImGui::InputInt("minimum skill level", &minimumSkillLevel);
	minimumSkillLevel = std::clamp(minimumSkillLevel, 0, SkillLevel::max().get());
	auto& hasCrafting = window.m_area->m_hasCraftingLocationsAndJobs.getForFaction(window.m_faction);
	if(ImGui::Button("Create"))
		hasCrafting.addJob(craftJobType, materialType, {quantity}, {minimumSkillLevel});
	ImGui::BeginTable("activeOrders", 3);
	for(const auto& [_craftJobType, _materialType, _quantity, craftJob] : collate(window))
	{
		ImGui::TextUnformatted(craftJob->describe().c_str());
		ImGui::TableNextColumn();
		if(_quantity != 1)
			ImGui::TextUnformatted(_quantity.toString().c_str());
		ImGui::TableNextColumn();
		if(ImGui::Button("-"))
			hasCrafting.removeJob(*craftJob);
		if(ImGui::Button("+"))
			hasCrafting.cloneJob(*craftJob);
		ImGui::TableNextRow();
	}
	ImGui::EndTable();
	end();
}