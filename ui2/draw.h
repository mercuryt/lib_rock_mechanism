#pragma once
#include "../engine/dataStructures/smallMap.h"
#include "../engine/pointFeature.h"
#include "displayData.h"
class SDL_Color;
class CuboidSet;
class Cuboid;
class Window;
struct Sprite;
enum class PointFeatureTypeId;
namespace draw
{
	void world(Window& window);
	void nonGroundCoverPlant(Window& window, const PlantIndex plant, const PlantSpeciesDisplayData& display);
	void featureType(Window& window, const PointFeatureTypeId type, const Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features);
	void rampOrStairs(Window& window, const PointFeatureTypeId type, const Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features);
	void featureTypeRotated90IfInaccessableNorthAndSouth(Window& window, const PointFeatureTypeId type, const Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features);
	void colorOnArea(Window& window, const CuboidSet& area, const SDL_Color color);
	void colorOnCuboid(Window& window, const Cuboid cuboid, const SDL_Color color);
	void colorOutlineArea(Window& window, const CuboidSet& cuboid, const SDL_Color color, const int thickness = 1);
	void colorOutlineCuboid(Window& window, const Cuboid cuboid, const SDL_Color color, const int thickness = 1);
	void colorOutlineCuboids(Window& window, const CuboidSet& cuboids, const SDL_Color color, const int thickness = 1);
	void textAtCoordinates(Window& window, const int x, const int y, const std::string& text, const SDL_Color color, const int size = displayData::textSize);
	void textAtPoint(Window& window, const Point3D point, const std::string& text, const SDL_Color color, const int size = displayData::textSize);
	void textOnCuboid(Window& window, const Cuboid cuboid, const std::string& text, const SDL_Color color, const int size = displayData::textSize);
	void actorAtLocation(Window& window, const ActorIndex actor);
	void itemAtLocation(Window& window, const ItemIndex item);
	void itemBeingCarried(Window& window, const ItemIndex item, const Point3D point);
	void selectedBoxes(Window& window);
	void itemOverlay(Window& window, const Point3D point);
	void actorOverlay(Window& window, const Point3D point);
	void accessableSymbol(Window& window, const Point3D point);
	void inaccessableSymbol(Window& window, const Point3D point);
	void validOnBlock(Window& window, const Point3D point);
	void invalidOnBlock(Window& window, const Point3D point);
	void progressBarOnBlock(Window& window, const Point3D point, const Percent progress);
	void maybeDesignated(Window& window, const Point3D point);
	[[nodiscard]] Facing4 rampOrStairsFacing(Window& window, const Point3D point);
};