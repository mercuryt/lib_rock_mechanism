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

struct PlantSpeciesDisplayData final
{
	std::string image;
	sf::Color color;
	float scale;
	bool groundCover;
};

struct AnimalSpeciesDisplayData final
{
	std::string image;
	sf::Color color;
	float scale;
};

struct ItemTypeDisplayData final
{
	std::string image;
	sf::Color color;
	float scale;
};

namespace displayData
{
	inline std::unordered_map<const MaterialType*, sf::Color> materialColors;
	inline std::unordered_map<const FluidType*, sf::Color> fluidColors;
	inline std::unordered_map<const ItemType*, ItemTypeDisplayData> itemData;
	inline std::unordered_map<const PlantSpecies*, PlantSpeciesDisplayData> plantData;
	inline std::unordered_map<const AnimalSpecies*, AnimalSpeciesDisplayData> actorData;
	inline float ratioOfScaleToFontSize;
	inline static const sf::Color selectColor = sf::Color::Yellow;
	inline constexpr uint32_t defaultScale = 32;
	inline constexpr float wallTopOffsetRatio = 0.18;
	inline constexpr float wallOffsetRatio = 0.00;
	inline constexpr float minimumFluidVolumeToSeeFromAboveLevelRatio = 0.75;
	void load();
}
