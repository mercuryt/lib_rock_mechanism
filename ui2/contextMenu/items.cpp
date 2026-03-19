#include "contextMenu.h"
#include "../window.h"
#include "../widgets/widgets.h"
#include "../../engine/items/items.h"
#include "../../engine/area/area.h"
#include "../../engine/space/space.h"
#include "../../engine/definitions/itemType.h"
void contextMenu::controlls::items(Window& window)
{
	if(ImGui::BeginMenu("items"))
	{
		ControllsState& state = window.m_gameOverlay.m_controllsState;
		Space& space = window.m_area->getSpace();
		Items& items = window.m_area->getItems();
		if(window.m_editMode)
		{
			widgets::itemType(&state.itemType);
			if(state.materialType.empty())
				state.materialType = MaterialType::byName("dirt");
			std::vector<MaterialCategoryTypeId>& categories = ItemType::getMaterialTypeCategories(state.itemType);
			auto materialTypeFilter = [&](const MaterialTypeId materialType){
				return std::ranges::find(categories, MaterialType::getMaterialTypeCategory(materialType)) != categories.end();
			};
			widgets::materialType(&state.materialType, std::move(materialTypeFilter));
			bool generic = ItemType::getGeneric(state.itemType);
			widgets::facing(&state.facing);
			if(generic)
			{
				ImGui::InputInt("quantity", &state.quantity.getReference());
				if(state.quantity < 1) state.quantity = {1};
			}
			else
			{
				ImGui::InputInt("percent wear", &state.wear.getReference());
				ImGui::InputInt("quality", &state.quality.getReference());
				if(ItemType::getInstallable(state.itemType))
					ImGui::Checkbox("installed", &state.installed);
				ImGui::InputText("name", &state.name);
				if(state.quality < 0) state.quality = {0};
				if(state.wear < 0) state.wear = {0};
			}
			if(ImGui::Button("create"))
			{
				ItemParamaters params{
					.itemType=state.itemType,
					.materialType=state.materialType,
					.faction=window.m_faction,
					.location=state.clickedOnPoint,
					.facing=state.facing,
				};
				if(generic)
					params.quantity = state.quantity;
				else
				{
					params.quality = state.quality;
					params.percentWear = state.wear;
					params.installed = state.installed;
					params.name = state.name;
				}
				window.m_area->getItems().create(params);
				ImGui::CloseCurrentPopup();
			}
			SmallSet<ItemIndex> selected;
			if(window.m_gameOverlay.m_selectMode == SelectMode::Space)
				selected = space.item_getAll(window.m_gameOverlay.m_selectedArea);
			else if(window.m_gameOverlay.m_selectMode == SelectMode::Items)
			{
				for(const ItemReference& ref : window.m_gameOverlay.m_selectedItems)
					selected.insert(ref.getIndex(items.m_referenceData));
			}
			else
				selected = space.item_getAll(window.m_gameOverlay.m_blockUnderCursor);
			if(ImGui::Button("destroy all"))
			{
				window.m_area->getItems().destroyAll(selected);
				ImGui::CloseCurrentPopup();
			}
			if(window.m_faction.exists() && selected.size() == 1)
			{
				const ItemIndex& item = selected.front();
				const Point3D& location = state.clickedOnPoint;
				if(location.exists() && location != window.m_gameOverlay.m_blockUnderCursor && space.shape_canEnterCurrentlyWithFacing(location, items.getShape(item), window.m_gameOverlay.m_facing, items.getOccupied(item)))
				{
					if(ImGui::Button("install"))
					{
						window.m_area->m_hasInstallItemDesignations.getForFaction(window.m_faction).add(*window.m_area, location, item, window.m_gameOverlay.m_facing, window.m_faction);
						ImGui::CloseCurrentPopup();
					}
				}
			}
			for(const ItemIndex& item : selected)
			{
				std::string description = items.description(item).c_str();
				if(ImGui::MenuItem((description + " info").c_str()))
				{
					window.m_gameOverlay.m_detailItem = items.getReference(item);
					window.m_gameOverlay.m_infoPopUp = InfoPopUpId::Item;
					ImGui::CloseCurrentPopup();
				}
				if(window.m_editMode)
					if(ImGui::Button(("destroy " + items.description(item)).c_str()))
					{
						items.destroy(item);
						ImGui::CloseCurrentPopup();
					}
				}
		}
		ImGui::EndMenu();
	}
}