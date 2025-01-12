#pragma once
#include "../engine/vectorContainers.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Window/Event.hpp>
#include <string>
struct MaterialTypeId;
struct ItemTypeId;
struct AnimalSpeciesId;
struct PlantSpeciesId;
struct FluidTypeId;
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
	inline SmallMap<MaterialTypeId, sf::Color> materialColors;
	inline SmallMap<FluidTypeId, sf::Color> fluidColors;
	inline SmallMap<ItemTypeId, ItemTypeDisplayData> itemData;
	inline SmallMap<PlantSpeciesId, PlantSpeciesDisplayData> plantData;
	inline SmallMap<AnimalSpeciesId, AnimalSpeciesDisplayData> actorData;
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

	std::wstring localizeNumber(double number);
	void load();
}
