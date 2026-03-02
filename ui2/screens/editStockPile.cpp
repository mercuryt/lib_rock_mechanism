#include "screens.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/area/area.h"
#include "../../engine/area/stockpile.h"

void screens::editStockPile(Window& window, StockPile* stockPile)
{
	begin(window, "Edit Stockpile");
	ImGui::BeginTable("queries", 4);
	// List queries.
	for(ItemQuery& itemQuery : stockPile->getQueries())
	{
		ImGuiText(ItemType::getName(itemQuery.m_itemType));
		ImGui::TableNextColumn();
		ImGuiText(MaterialType::getName(itemQuery.m_solid));
		ImGui::TableNextColumn();
		if(ImGui::Button("destroy"))
			stockPile->removeQuery(itemQuery);
		ImGui::TableNextRow();
	}
	ImGui::EndTable();
	// Create new query.
	ItemTypeId itemTypeId;
	MaterialTypeId materialTypeId;
	MaterialCategoryTypeId materialCategoryTypeId;
	widgets::itemTypeAndMaterialTypeOrCategory(&itemTypeId, &materialTypeId, &materialCategoryTypeId);
	if(ImGui::Button("create"))
	{
		auto query = ItemQuery::create(itemTypeId, materialTypeId, materialCategoryTypeId);
		if(!stockPile->contains(query))
			window.m_area->m_hasStockPiles.getForFaction(window.m_faction).addQuery(*stockPile, query);
	}
	if(ImGui::Button("back"))
		window.showGame();
	end();
}
