#include "materialType.h"
#include "item.h"
#include <algorithm>
#include <ranges>
MaterialConstructionData& MaterialConstructionData::byNameSpecialized(const std::string name, const MaterialType& materialType)
{
	auto found = std::ranges::find(data, name, &MaterialConstructionData::name);
	assert(found != data.end());
	MaterialConstructionData output = *found;
	for(auto& [itemQuery, quantity] : output.consumed)
		if(itemQuery.m_materialType == nullptr)
			itemQuery.m_materialType = &materialType;
	for(auto& tuple : output.byproducts)
		if(std::get<1>(tuple) == nullptr)
			std::get<1>(tuple) = &materialType;
	specializedData.push_back(output);
	return specializedData.back();
}
