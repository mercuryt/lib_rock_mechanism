#pragma once

#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>
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
	inline float wallOffset;
	inline const sf::Color selectColor = sf::Color::Yellow;
	inline constexpr uint32_t defaultScale = 32;
	inline constexpr float wallTopOffsetRatio = 0.18;
	inline constexpr float minimumFluidVolumeToSeeFromAboveLevelRatio = 0.75;
	inline sf::Color stockPileColor{168, 127, 50, 64};
	inline sf::Color farmFieldColor{133, 82, 38, 64};
	inline sf::Color contextMenuHoverableColor = sf::Color::Blue;
	inline sf::Color contextMenuUnhoverableColor = sf::Color::Black;
	inline sf::Color actorOutlineColor = sf::Color::Blue;
	inline sf::Color progressBarColor;
	inline sf::Color progressBarOutlineColor;
	inline float progressBarThickness;
	inline std::size_t maximumNumberOfItemsToDisplayInComboBox;
	inline sf::Mouse::Button selectMouseButton = sf::Mouse::Button::Left;
	inline sf::Mouse::Button actionMouseButton = sf::Mouse::Button::Right;

	std::string localizeNumber(double number);
	void load();
}
