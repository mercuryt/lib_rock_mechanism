#include "materialType.h"
#include <algorithm>
#include <ranges>
const MaterialCategoryTypeId MaterialTypeCategory::byName(const std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return MaterialCategoryTypeId::create(found - data.m_name.begin());
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
MaterialTypeId MaterialType::byName(const std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return MaterialTypeId::create(found - data.m_name.begin());
}
