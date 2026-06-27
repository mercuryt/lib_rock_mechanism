#include "screens.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/uniform.h"

void screens::editUniform(Window& window, Uniform* uniform)
{
	std::string title = uniform != nullptr ? "Edit Uniform" : "Create Uniform";
	begin(window, title);
	ImGui::InputText("name", &uniform->name);
	ImGui::BeginTable("elements", 4);
	for(const UniformElement& element : uniform->elements)
	{
		ImGui::TextUnformatted(ItemType::getName(element.m_itemType).c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(MaterialType::getName(element.m_solid).c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(MaterialTypeCategory::getName(element.m_materialCategoryType).c_str());
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(element.m_quantity.toS().c_str());
		ImGui::TableNextColumn();
		if(ImGui::Button("destroy"))
			window.m_simulation->m_hasUniforms.getForFaction(window.m_faction).destroyUniform(*uniform);
		ImGui::TableNextRow();
	}
	ImGui::EndTable();
	// Create new element.
	ItemTypeId itemTypeId;
	MaterialTypeId materialTypeId;
	MaterialCategoryTypeId materialCategoryTypeId;
	int quantity = 1;
	widgets::itemTypeAndMaterialTypeOrCategory(&itemTypeId, &materialTypeId, &materialCategoryTypeId);
	ImGui::InputInt("quantity",&quantity);
	if(quantity < 1) quantity = 1;
	if(ImGui::Button("create"))
	{
		bool invalid = itemTypeId.empty() || (materialTypeId.empty() && materialCategoryTypeId.empty());
		if(!invalid)
			uniform->elements.emplace_back(itemTypeId, materialCategoryTypeId, materialTypeId, Quantity::create(quantity));
	}
	if(ImGui::Button("back"))
		window.showGame();
	end();
}
