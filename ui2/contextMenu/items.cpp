#include "contextMenu.h"
#include "../window.h"
void contextMenu::item(Window& window)
{
	ContextMenuState& state = window.m_gameOverly.m_contextMenuState;
	widgets::itemType(&state.itemType);
	widgets::materialType(&state.materialType);
	widgets::facing(&state.facing);
	ImGui::InputInt("percent wear", &state.wear.get());
	ImGui::InputInt("quality", &state.quality.get());
	ImGui::InputInt("quantity", &state.quantity.get());
	ImGui::CheckBox("installed", &state.installed);
	ImGui::InputText("name", &state.name);
	if(state.quality < 0) state.quality = {0};
	if(state.wear < 0) state.wear = {0};
	if(state.quantity < 1) state.quantity = {1};
	SmallSet<ItemReference> selected;
	Space& space = window.m_area->getSpace();
	Items& items = window.m_area->getItems();
	if(window.m_gameOverlay.m_selectMode == SelectMode::Space)
		selected = space.items_getAll(window.m_gameOverlay.m_selectedArea);
	else if(window.m_gameOverlay.m_selectMode == SelectMode::Items)
		selected = window.m_maveOverlay.m_selectedItems;
	else
		selected = space.items_getAll(window.m_gameOverlay.m_blockUnderCursor);
	if(window.m_editMode)
	{
		if(ImGui::Button("create"))
		{
			window.m_area->getItems().create({
				.itemType=state.itemType,
				.materialType=state.materialType,
				.faction=window.m_faction,
				.location=window.m_gameOverlay.m_blockUnderCursor,
				.quantity=state.quantity,
				.quality=state.quality,
				.wear=state.wear.quality=state.quality,
				.percentWear=state.wear,
				.facing=state.facing,
				.isInstalled=state.installed,
				.name=state.name
			});
			state.current = ContextMenuId::Null;
		}
		if(ImGui::Button("destroy all"))
		{
			window.m_area->getItems().destroyAll(items);
			state.current = ContextMenuId::Null;
		}
		for(const ItemReference& itemRef : selected)
		{
			const ItemIndex item = itemRef.getIndex(items.m_referenceData);
			ImGui::Text(items.toString(item));
			if(window.m_editMode)
				if(ImGui::Button("destroy"))
				{
					items.destroy(item);
				}
			if(window.m_faction.exists())
			{
				if(ImGui::Button("install"))
				{
					const ItemIndex& item = selected.front().getIndex(items.m_referenceData);
					m_window.getArea()->m_hasInstallItemDesignations.getForFaction(faction).add(area, point, item, facing, faction);
					state.current = ContextMenuId::Null;
				}
			}
		}
	}
}