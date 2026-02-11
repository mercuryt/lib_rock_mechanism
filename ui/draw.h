#pragma once
#include "../engine/numericTypes/types.h"
#include "../engine/dataStructures/smallMap.h"
#include "../engine/geometry/cuboidSet.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
class Window;
class Point3D;
class ItemIndex;
class PlantIndex;
class ActorIndex;
struct Cuboid;
class PointFeature;
class PlantSpeciesDisplayData;
class Draw final
{
	Window& m_window;
public:
	Draw(Window& w) : m_window(w) { }
	void view();
	void blockWallCorner(const Point3D& point, const bool& constructed, const bool& nextZLevelDown);
	void blockWalls(const Cuboid& cuboid, const MaterialTypeId& materialType, const bool& constructed, const bool& nextZLevelDown);
	void blockWallsSouth(const CuboidSet& cuboids, const sf::Color& color, const sf::Texture& texture, const bool& nextZLevelDown);
	void blockWallsWest(const CuboidSet& cuboids, const sf::Color& color, const sf::Texture& texture, const bool& nextZLevelDown);
	void validOnBlock(const Point3D& point);
	void invalidOnBlock(const Point3D& point);
	void colorOnCuboid(const Cuboid& cuboid, const sf::Color& color);
	void colorOnCuboids(const std::vector<Cuboid>& cuboids, const sf::Color& color);
	void colorOnBlock(const Point3D& point, const sf::Color& color);
	void maybeDesignated(const Point3D& point);
	sf::Sprite getCenteredSprite(std::string name);
	void featureType(const sf::Texture& texture, const PointFeatureTypeId& featureTypeId, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features);
	void rampOrStairs(const PointFeatureTypeId& type, sf::Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features);
	void featureTypeRotated90IfInaccessableNorthAndSouth(const PointFeatureTypeId& type, sf::Sprite& sprite, const SmallMap<PointFeatureTypeId, SmallMap<PointFeature, CuboidSet>>& features);
	void rampOrStairsTopOnly(const Cuboid& cuboid, sf::Sprite& sprite, const sf::Color& color);
	void nonGroundCoverPlant(const PlantIndex& plant, const PlantSpeciesDisplayData& display);
	void item(const Point3D& point);
	void itemOverlay(const Point3D& point);
	void item(const ItemIndex& item, sf::Vector2f position);
	void itemOverlay(const ItemIndex& item, sf::Vector2f position);
	void singleTileActor(const ActorIndex& actor);
	void multiTileActor(const ActorIndex& actor);
	void actorOverlay(const ActorIndex& actor);
	void multiTileBorder(const CuboidSet& blocksOccpuied, sf::Color color, float thickness);
	void borderSegmentOnBlock(const Point3D& point, const Facing4& facing, sf::Color color, float thickness);
	void accessableSymbol(const Point3D& point);
	void inaccessableSymbol(const Point3D& point);
	void progressBarOnBlock(const Point3D& point, Percent progress);
	void textureFill(const sf::Texture& texture, const Cuboid& cuboid, const sf::Color* color = nullptr);
	void spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color = nullptr);
	void spriteAtWithScale(sf::Sprite& sprite, sf::Vector2f position, float scale, const sf::Color* color = nullptr);
	void spriteFill(sf::Sprite& sprite, const Cuboid& cuboid, const sf::Color* color = nullptr);
	void spriteFillCentered(sf::Sprite& sprite, const Cuboid& cuboid, const sf::Color* color = nullptr);
	void spriteFillWithScale(sf::Sprite& sprite, const Cuboid& cuboid, float scale, const sf::Color* color = nullptr);
	void spriteFillCenteredWithScale(sf::Sprite& sprite, const Cuboid& cuboid, float scale, const sf::Color* color = nullptr);
	void spriteOnBlockWithScaleCentered(const Point3D& point, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlockWithScale(const Point3D& point, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlock(const Point3D& point, sf::Sprite& sprite, const sf::Color* = nullptr);
	void spriteOnBlockCentered(const Point3D& point, sf::Sprite& sprite, const sf::Color* = nullptr);
	void imageOnBlock(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void imageOnBlockWestAlign(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void imageOnBlockEastAlign(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void imageOnBlockSouthAlign(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void selected(const Point3D& point);
	void outlineOnBlock(const Point3D& point, const sf::Color color, float thickness = 3.f);
	void stringAtPosition(const std::string string, const sf::Vector2f position, const sf::Color color, float offsetX = 0.5, float offsetY = 0.0);
	void stringOnBlock(const Point3D& point,const  std::string string, const sf::Color color, float offsetX = 0.5, float offsetY = 0.0);
	// Connects to an open top point, tries to align with an open bottom point.
	[[nodiscard]] Facing4 rampOrStairsFacing(const Point3D& point) const;
	[[nodiscard]] sf::Vector2f blockToPosition(const Point3D& point) const;
	[[nodiscard]] sf::Vector2f blockToPositionCentered(const Point3D& point) const;
	[[nodiscard]] CuboidSet getWallsToDrawWest(const Cuboid& cuboid);
	[[nodiscard]] CuboidSet getWallsToDrawSouth(const Cuboid& cuboid);
	[[nodiscard]] bool doDrawCorner(const Point3D& point) const;
};
