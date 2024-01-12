#pragma once

#include <SFML/Graphics/Color.hpp>
#include <string>
#include <unordered_map>
struct MaterialType;
struct ItemType;
struct AnimalSpecies;
struct PlantSpecies;
struct FluidType;
struct BlockFeatureType;

namespace displayData
{
	inline std::unordered_map<const MaterialType*, sf::Color> materialColors;
	inline std::unordered_map<const FluidType*, sf::Color> fluidColors;
	inline std::unordered_map<const ItemType*, std::wstring> itemSymbols;
	inline std::unordered_map<const PlantSpecies*, std::wstring> plantSymbols;
	inline std::unordered_map<const AnimalSpecies*, std::wstring> actorSymbols;
	inline std::unordered_map<const AnimalSpecies*, sf::Color> actorColors;
	inline std::unordered_map<const BlockFeatureType*, std::wstring> blockFeatureSymbols;
	void load();
}
