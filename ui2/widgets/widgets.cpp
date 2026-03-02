#include "widgets.h"
#include "../window.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/fluidType.h"
#include "../../engine/definitions/plantSpecies.h"
#include "../../engine/pointFeature.h"
#include "../../engine/numericTypes/types.h"
#include "../../engine/numericTypes/idTypes.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/faction.h"
#include "../../engine/craft.h"
bool widgets::materialType(MaterialTypeId* result, std::function<bool(const MaterialTypeId)>&& condition)
{
	bool output = false;
	ImGui::BeginCombo("Material", MaterialType::getName(*result).c_str());
	auto end = MaterialType::size();
	for(MaterialTypeId materialTypeId{0}; materialTypeId != end; ++materialTypeId)
	{
		if(condition != nullptr && !condition(materialTypeId))
			continue;
		bool isSelected = result != nullptr && *result == materialTypeId;
		if(ImGui::Selectable(MaterialType::getName(materialTypeId).c_str(), isSelected))
		{
			(*result) = materialTypeId;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::materialCategoryType(MaterialCategoryTypeId* result, std::function<bool(const MaterialCategoryTypeId)>&& condition)
{
	bool output = false;
	ImGui::BeginCombo("Material", MaterialTypeCategory::getName(*result).c_str());
	for(MaterialCategoryTypeId id{0}; id < MaterialTypeCategory::size(); ++id)
	{
		if(condition != nullptr && !condition(id))
			continue;
		std::string name = MaterialTypeCategory::getName(id);
		bool isSelected = id == *result;
		if(ImGui::Selectable(name.c_str(), isSelected))
		{
			(*result) = id;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::featureType(PointFeatureTypeId* result, std::function<bool(const PointFeatureTypeId)>&& condition)
{
	bool output = false;
	ImGui::BeginCombo("", PointFeatureType::byId(*result).name.c_str());
	for(const PointFeatureType* pointFeatureType : PointFeatureType::getAll())
	{
		PointFeatureTypeId id = PointFeatureType::getId(*pointFeatureType);
		if(condition != nullptr && !condition(id))
			continue;
		bool isSelected = result != nullptr && *result == id;
		if(ImGui::Selectable(pointFeatureType->name.c_str(), isSelected))
		{
			(*result) = id;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::fluidType(FluidTypeId* result, std::function<bool(const FluidTypeId)>&& condition)
{
	bool output = false;
	ImGui::BeginCombo("fluid", FluidType::getName(*result).c_str());
	auto end = FluidType::size();
	for(FluidTypeId fluidTypeId{0}; fluidTypeId != end; ++fluidTypeId)
	{
		if(condition != nullptr && !condition(fluidTypeId))
			continue;
		bool isSelected = result != nullptr && *result == fluidTypeId;
		if(ImGui::Selectable(FluidType::getName(fluidTypeId).c_str(), isSelected))
		{
			(*result) = fluidTypeId;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::plantSpecies(PlantSpeciesId* result, std::function<bool(const PlantSpeciesId)>&& condition)
{
	bool output = false;
	ImGui::BeginCombo("plant species", PlantSpecies::getName(*result).c_str());
	auto end = PlantSpecies::size();
	for(PlantSpeciesId plantSpeciesId{0}; plantSpeciesId != end; ++plantSpeciesId)
	{
		if(condition != nullptr && !condition(plantSpeciesId))
			continue;
		bool isSelected = result != nullptr && *result == plantSpeciesId;
		if(ImGui::Selectable(PlantSpecies::getName(plantSpeciesId).c_str(), isSelected))
		{
			(*result) = plantSpeciesId;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::itemType(ItemTypeId* result, std::function<bool(const ItemTypeId)>&& condition)
{
	bool output = false;
	ImGui::BeginCombo("item type", ItemType::getName(*result).c_str());
	auto end = ItemType::size();
	for(ItemTypeId itemType{0}; itemType != end; ++itemType)
	{
		if(condition != nullptr && !condition(itemType))
			continue;
		bool isSelected = result != nullptr && *result == itemType;
		if(ImGui::Selectable(ItemType::getName(itemType).c_str(), isSelected))
		{
			(*result) = itemType;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::faction(FactionId* result, const Simulation& simulation, std::function<bool(const FactionId)>&& condition)
{
	bool output = false;
	std::string text = result == nullptr ? "" : simulation.m_hasFactions.getById(*result).name;
	ImGui::BeginCombo("faction", text.c_str());
	for(const Faction& faction : simulation.m_hasFactions.getAll())
	{
		if(condition != nullptr && !condition(faction.id))
			continue;
		bool isSelected = result != nullptr && *result == faction.id;
		if(ImGui::Selectable(faction.name.c_str(), isSelected))
		{
			(*result) = faction.id;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::facing(Facing4* result)
{
	bool output = false;
	const char* options[] = {"north", "east", "south", "west"};
	ImGui::BeginCombo("item type", options[(int)*result]);
	for(int i{0}; i != 4; ++i)
	{
		bool isSelected = result != nullptr && (int)*result == i;
		if(ImGui::Selectable(options[i], isSelected))
		{
			(*result) = (Facing4)i;
			output = true;
		}
		if(isSelected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}
bool widgets::itemTypeAndMaterialTypeOrCategory(ItemTypeId* itemType, MaterialTypeId* materialType, MaterialCategoryTypeId* categoryType, std::function<bool(const ItemTypeId)>&& condition)
{
	bool output = false;
	if(widgets::itemType(itemType, std::move(condition)))
		output = true;
	auto& materialTypeCategoriesForItemType = ItemType::getMaterialTypeCategories(*itemType);
	auto materialTypeCondition =[&](const MaterialTypeId materialTypeId) {
		return std::ranges::find(materialTypeCategoriesForItemType, MaterialType::getMaterialTypeCategory(materialTypeId)) != materialTypeCategoriesForItemType.end();
	};
	if(widgets::materialType(materialType, materialTypeCondition))
		output = true;
	auto materialCategoryCondition =[&](const MaterialCategoryTypeId materialCategoryTypeId) {
		return std::ranges::find(materialTypeCategoriesForItemType, materialCategoryTypeId) != materialTypeCategoriesForItemType.end();
	};
	if(widgets::materialCategoryType(categoryType, materialCategoryCondition))
		output = true;
	return output;
}
bool widgets::craftJobType(CraftJobTypeId* result)
{
	bool output = false;
	std::string selectedName = CraftJobType::getName(*result);
	ImGui::BeginCombo("craftJobType", selectedName.c_str());
	for(auto craftJobTypeId = CraftJobTypeId::create(0); craftJobTypeId < CraftJobType::size(); ++craftJobTypeId)
	{
		std::string name = CraftJobType::getName(craftJobTypeId);
		bool selected = name == selectedName;
		if(ImGui::Selectable(name.c_str(), selected))
		{
			(*result) = CraftJobType::byName(name);
			output = true;
		}
		if(selected)
			ImGui::SetItemDefaultFocus();
	}
	ImGui::EndCombo();
	return output;
}