#include "materialType.h"
#include <algorithm>
#include <ranges>
const MaterialTypeCategory& MaterialTypeCategory::byName(const std::string name)
{
	auto found = std::ranges::find(materialTypeCategoryDataStore, name, &MaterialTypeCategory::name);
	assert(found != materialTypeCategoryDataStore.end());
	return *found;
}
MaterialConstructionData& MaterialConstructionData::byNameSpecialized(const std::string name, const MaterialType& materialType)
{
	auto found = std::ranges::find(materialConstructionDataStore, name, &MaterialConstructionData::name);
	assert(found != materialConstructionDataStore.end());
	MaterialConstructionData output = *found;
	for(auto& [itemQuery, quantity] : output.consumed)
		if(itemQuery.m_materialType == nullptr)
			itemQuery.m_materialType = &materialType;
	for(auto& tuple : output.byproducts)
		if(std::get<1>(tuple) == nullptr)
			std::get<1>(tuple) = &materialType;
	materialConstructionSpecializedDataStore.push_back(output);
	return materialConstructionSpecializedDataStore.back();
}
const MaterialType& MaterialType::byName(const std::string name)
{
	auto found = std::ranges::find(materialTypeDataStore, name, &MaterialType::name);
	assert(found != materialTypeDataStore.end());
	return *found;
}
MaterialType& MaterialType::byNameNonConst(const std::string name)
{
	auto found = std::ranges::find(materialTypeDataStore, name, &MaterialType::name);
	assert(found != materialTypeDataStore.end());
	return *found;
}
