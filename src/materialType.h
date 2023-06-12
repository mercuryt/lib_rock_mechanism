#pragma once

#include <string>

template<class MaterialType, class ItemType>
struct SpoilData
{
	const MaterialType& materialType;
	const ItemType& itemType;
	double chance;
	uint32_t min;
	uint32_t max;
}

template<class DerivedMaterialType>
struct BaseMaterialType
{
	const std::string name;
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
};
