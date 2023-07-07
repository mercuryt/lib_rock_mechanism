#pragma once

#include <string>
#include <vector>

struct MaterialType;
struct ItemType;

struct MaterialTypeCategory
{
	const std::string name;
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
	// How does this material dig?
	const std::vector<SpoilData> spoilData;
	// Infastructure.
	bool operator==(const MaterialType& materialType){ return this == &materialType; }
	static std::vector<MaterialType> data;
	static const MaterialType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &MaterialType::name);
		assert(found != data.end());
		return *found;
	}
};
