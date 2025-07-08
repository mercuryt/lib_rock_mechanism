#pragma once
#include "../engine/numericTypes/types.h"
#include "../engine/hasShapeTypes.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
class Window;
class Point3D;
class ItemIndex;
class PlantIndex;
class ActorIndex;
class Cuboid;
class Draw final
{
	Window& m_window;
public:
	Draw(Window& w) : m_window(w) { }
	void view();
	void blockFloor(const Point3D& point);
	void blockWallCorners(const Point3D& point);
	void blockWalls(const Point3D& point);
	void blockWallTops(const Point3D& point);
	void pointFeaturesAndFluids(const Point3D& point);
	void blockWallsFromNextLevelDown(const Point3D& point);
	void validOnBlock(const Point3D& point);
	void invalidOnBlock(const Point3D& point);
	void colorOnBlock(const Point3D& point, const sf::Color color);
	void maybeDesignated(const Point3D& point);
	void craftLocation(const Point3D& point);
	sf::Sprite getCenteredSprite(std::string name);

	void spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color = nullptr);
	void spriteAtWithScale(sf::Sprite& sprite, sf::Vector2f position, float scale, const sf::Color* color = nullptr);
	void spriteOnBlockWithScaleCentered(const Point3D& point, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlockWithScale(const Point3D& point, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlock(const Point3D& point, sf::Sprite& sprite, const sf::Color* = nullptr);
	void spriteOnBlockCentered(const Point3D& point, sf::Sprite& sprite, const sf::Color* = nullptr);
	void imageOnBlock(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void imageOnBlockWestAlign(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void imageOnBlockEastAlign(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void imageOnBlockSouthAlign(const Point3D& point, std::string name, const sf::Color* = nullptr);
	void progressBarOnBlock(const Point3D& point, Percent progress);

	void selected(const Point3D& point);
	void selected(const Cuboid& cuboid);
	void outlineOnBlock(const Point3D& point, const sf::Color color, float thickness = 3.f);
	void stringAtPosition(const std::wstring string, const sf::Vector2f position, const sf::Color color, float offsetX = 0.5, float offsetY = 0.0);
	void stringOnBlock(const Point3D& point,const  std::wstring string, const sf::Color color, float offsetX = 0.5, float offsetY = 0.0);

	void nonGroundCoverPlant(const Point3D& point);
	void item(const Point3D& point);
	void itemOverlay(const Point3D& point);
	void item(const ItemIndex& item, sf::Vector2f position);
	void itemOverlay(const ItemIndex& item, sf::Vector2f position);
	void singleTileActor(const ActorIndex& actor);
	void multiTileActor(const ActorIndex& actor);
	void actorOverlay(const ActorIndex& actor);
	void multiTileBorder(const OccupiedSpaceForHasShape& blocksOccpuied, sf::Color color, float thickness);
	void borderSegmentOnBlock(const Point3D& point, const Facing4& facing, sf::Color color, float thickness);
	void accessableSymbol(const Point3D& point);
	void inaccessableSymbol(const Point3D& point);

	// Connects to an open top point, tries to align with an open bottom point.
	[[nodiscard]] Facing4 rampOrStairsFacing(const Point3D& point) const;
	[[nodiscard]] sf::Vector2f blockToPosition(const Point3D& point) const;
	[[nodiscard]] sf::Vector2f blockToPositionCentered(const Point3D& point) const;
	[[nodiscard]] float getScaledUnit() const;
};
