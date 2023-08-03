#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

struct MaterialType;
struct ItemType;
class ItemQuery;
struct FluidType;

struct MaterialTypeCategory
{
	const std::string name;
	// Infastructure.
	bool operator==(const MaterialTypeCategory& materialTypeCategory) const { return this == &materialTypeCategory; }
	inline static std::vector<MaterialTypeCategory> data;
	static const MaterialTypeCategory& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &MaterialTypeCategory::name);
		assert(found != data.end());
		return *found;
	}
};

struct SpoilData
{
	const MaterialType& materialType;
	const ItemType& itemType;
	const double chance;
	const uint32_t min;
	const uint32_t max;
};

struct BurnData
{
};

struct MaterialConstructionData
{
	std::vector<std::pair<ItemQuery, uint32_t>> consumed;
	std::vector<std::pair<ItemQuery, uint32_t>> unconsumed;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> byproducts;
};

struct MaterialType
{
	const std::string name;
	const MaterialTypeCategory materialTypeCategory;
	const uint32_t density;
	const uint32_t hardness;
	const bool transparent;
	// How does this material burn?
	const uint32_t ignitionTemperature; // Temperature at which this material catches fire.
	const uint32_t flameTemperature; // Temperature given off by flames from this material.
	const uint32_t burnStageDuration; // How many steps to go from smouldering to burning and from burning to flaming.
	const uint32_t flameStageDuration; // How many steps to spend flaming.
	// What temperatures cause phase changes?
	const uint32_t meltingPoint;
	const FluidType* meltsInto;
	// How does this material dig?
	std::vector<SpoilData> spoilData;
	BurnData* burnData;
	MaterialConstructionData* constructionData;
	// Infastructure.
	bool operator==(const MaterialType& materialType) const { return this == &materialType; }
	inline static std::vector<MaterialType> data;
	static const MaterialType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &MaterialType::name);
		assert(found != data.end());
		return *found;
	}
};
