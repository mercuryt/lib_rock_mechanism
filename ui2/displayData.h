#pragma once
// TODO: explicit instantiation
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
	SDL_Color color;
	float scale;
	bool groundCover;
};

struct AnimalSpeciesDisplayData final
{
	std::string image;
	SDL_Color color;
	float scale;
};

struct ItemTypeDisplayData final
{
	std::string image;
	SDL_Color color;
	float scale;
};

namespace displayData
{
	inline SmallMap<MaterialTypeId, SDL_Color> materialColors;
	inline SmallMap<FluidTypeId, SDL_Color> fluidColors;
	inline SmallMap<ItemTypeId, ItemTypeDisplayData> itemData;
	inline SmallMap<PlantSpeciesId, PlantSpeciesDisplayData> plantData;
	inline SmallMap<AnimalSpeciesId, AnimalSpeciesDisplayData> actorData;
	inline float ratioOfScaleToFontSize;
	inline float wallOffset;
	inline constexpr SDL_Color selectColor{255,255,0,255}; // Yellow
	inline constexpr SDL_Color cancelColor{255,0,0,255}; // Red
	inline constexpr SDL_Color selectColorOverlay{255, 255, 0, 128}; // Half transparent yellow
	inline constexpr SDL_Color cancelColorOverlay{255, 0, 0, 128}; // Half transparent red
	inline constexpr SDL_Color unrevealedColor{0,0,0,255}; //Black
	inline constexpr int defaultScale(32);
	inline constexpr float wallTopOffsetRatio(0.18);
	inline constexpr float minimumFluidVolumeToSeeFromAboveLevelRatio(0.75);
	inline constexpr SDL_Color stockPileColor{168, 127, 50, 64};
	inline constexpr SDL_Color farmFieldColor{133, 82, 38, 64};
	inline constexpr SDL_Color contextMenuHoverableColor{0,0,255,255}; // Blue
	inline constexpr SDL_Color contextMenuUnhoverableColor{0,0,0,255}; // Black
	inline constexpr SDL_Color actorOutlineColor{0,0,255,255}; // Blue;
	inline constexpr SDL_Color progressBarColor{0,0,255,255}; // Blue
	inline constexpr SDL_Color progressBarOutlineColor{0,0,0,255}; // Black
	inline constexpr SDL_Color areaOutlineColor{255,255,255,255}; // White
	inline constexpr float progressBarThickness(2.f);
	inline constexpr float zoomIncrement(0.1);
	inline std::size_t maximumNumberOfItemsToDisplayInComboBox;
	inline constexpr auto selectMouseButton = SDL_BUTTON_LEFT;
	inline constexpr auto actionMouseButton = SDL_BUTTON_RIGHT;
	inline constexpr float menuFontSize(30.f);
	inline constexpr float panSpeed(20.f);

	std::string localizeNumber(double number);
	void load();
}
