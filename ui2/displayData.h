#pragma once
// TODO: explicit instantiation
#include "sprite.h"
#include "../engine/dataStructures/smallSet.hpp"
#include "../engine/dataStructures/smallMap.hpp"
#include <SDL2/SDL.h>
#include <string>
struct MaterialTypeId;
struct ItemTypeId;
struct AnimalSpeciesId;
struct PlantSpeciesId;
struct FluidTypeId;
struct PointFeatureType;

struct PlantSpeciesDisplayData final
{
	std::string image;
	Sprite sprite;
	SDL_Color color;
	float scale;
	bool groundCover;
};

struct AnimalSpeciesDisplayData final
{
	std::string image;
	Sprite sprite;
	SDL_Color color;
	float scale;
};

struct ItemTypeDisplayData final
{
	std::string image;
	Sprite sprite;
	SDL_Color color;
	float scale;
};

enum class TemperatureDisplayUnits { F,C,K };
NLOHMANN_JSON_SERIALIZE_ENUM(TemperatureDisplayUnits, {
	{TemperatureDisplayUnits::F, "F"},
	{TemperatureDisplayUnits::C, "C"},
	{TemperatureDisplayUnits::K, "K"}
});

namespace displayData
{
	inline SmallMap<MaterialTypeId, SDL_Color> materialColors;
	inline SmallMap<FluidTypeId, SDL_Color> fluidColors;
	inline SmallMap<ItemTypeId, ItemTypeDisplayData> itemData;
	inline SmallMap<PlantSpeciesId, PlantSpeciesDisplayData> plantData;
	inline SmallMap<AnimalSpeciesId, AnimalSpeciesDisplayData> actorData;
	inline float ratioOfScaleToFontSize;
	inline float wallOffset;
	inline int offsetForWallsOnThisLevel{8};
	inline TemperatureDisplayUnits temperatureDisplayUnits;
	inline constexpr SDL_Color selectColor{255,255,0,255}; // Yellow
	inline constexpr SDL_Color cancelColor{255,0,0,255}; // Red
	inline constexpr int selectBoxThickness(2);
	inline constexpr SDL_Color selectColorOverlay{255, 255, 0, 128}; // Half transparent yellow
	inline constexpr SDL_Color cancelColorOverlay{255, 0, 0, 128}; // Half transparent red
	inline constexpr SDL_Color unrevealedColor{0,0,0,255}; //Black
	inline constexpr SDL_Color fluidVolumeTextColor{0,0,0,255}; //Black
	inline constexpr SDL_Color itemTypeCountColor{255,0,255,255}; //Magenta
	inline constexpr SDL_Color itemQuantityColor{255,255,255,255}; //White
	inline constexpr SDL_Color sleepZZZColor{0,255,255,255}; //Cyan
	inline constexpr int defaultScale(32);
	inline constexpr float wallTopOffsetRatio(0.18);
	inline constexpr float minimumFluidVolumeToSeeFromAboveLevelRatio(0.75);
	inline constexpr SDL_Color stockPileColor{168, 127, 50, 64};
	inline constexpr SDL_Color farmFieldColor{133, 82, 38, 64};
	inline constexpr SDL_Color contextMenuHoverableColor{0,0,255,255}; // Blue
	inline constexpr SDL_Color contextMenuUnhoverableColor{0,0,0,255}; // Black
	inline constexpr SDL_Color actorOutlineColor{0,0,255,255}; // Blue;
	inline constexpr int actorOutlineThickness{1};
	inline constexpr SDL_Color itemOutlineColor{150,75,0,255}; // Brown;
	inline constexpr int itemOutlineThickness{1};
	inline constexpr SDL_Color progressBarColor{0,0,255,255}; // Blue
	inline constexpr SDL_Color progressBarBackgroundColor{0,0,0,255}; // white
	inline constexpr SDL_Color progressBarOutlineColor{0,0,0,255}; // Black
	inline constexpr SDL_Color areaOutlineColor{255,255,255,255}; // White
	inline constexpr SDL_Color allowedColor{0,255,0,255}; // Green
	inline constexpr SDL_Color notAllowedColor{255,0,0,255}; // Red
	inline constexpr int areaOutlineWidth(4);
	inline constexpr int progressBarThickness(10);
	inline constexpr int progressBarOutlineThickness(1);
	inline constexpr float zoomIncrement(0.1f);
	inline std::size_t maximumNumberOfItemsToDisplayInComboBox;
	inline constexpr auto selectMouseButton = SDL_BUTTON_LEFT;
	inline constexpr auto actionMouseButton = SDL_BUTTON_RIGHT;
	inline constexpr float menuFontSize(30.f);
	inline constexpr float panSpeed(20.f);
	inline constexpr int selectThickness(2);
	inline constexpr int textSpacing(2);
	inline constexpr int textSize(16);

	std::string localizeNumber(double number);
	std::string formatTemperature(Temperature temperature);
	void load();
}
