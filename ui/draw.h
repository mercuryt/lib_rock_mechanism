#pragma once
#include "../engine/types.h"
#include "../engine/hasShapeTypes.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
class Window;
class BlockIndex;
class ItemIndex;
class PlantIndex;
class ActorIndex;
class Draw final
{
	Window& m_window;
public:
	Draw(Window& w) : m_window(w) { }
	void view();
	void blockFloor(const BlockIndex& block);
	void blockWallCorners(const BlockIndex& block);
	void blockWalls(const BlockIndex& block);
	void blockWallTops(const BlockIndex& block);
	void blockFeaturesAndFluids(const BlockIndex& block);
	void blockWallsFromNextLevelDown(const BlockIndex& block);
	void validOnBlock(const BlockIndex& block);
	void invalidOnBlock(const BlockIndex& block);
	void colorOnBlock(const BlockIndex& block, const sf::Color color);
	void designated(const BlockIndex& block);
	void craftLocation(const BlockIndex& block);
	sf::Sprite getCenteredSprite(std::string name);

	void spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color = nullptr);
	void spriteAtWithScale(sf::Sprite& sprite, sf::Vector2f position, float scale, const sf::Color* color = nullptr);
	void spriteOnBlockWithScaleCentered(const BlockIndex& block, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlockWithScale(const BlockIndex& block, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlock(const BlockIndex& block, sf::Sprite& sprite, const sf::Color* = nullptr);
	void spriteOnBlockCentered(const BlockIndex& block, sf::Sprite& sprite, const sf::Color* = nullptr);
	void imageOnBlock(const BlockIndex& block, std::string name, const sf::Color* = nullptr);
	void imageOnBlockWestAlign(const BlockIndex& block, std::string name, const sf::Color* = nullptr);
	void imageOnBlockEastAlign(const BlockIndex& block, std::string name, const sf::Color* = nullptr);
	void imageOnBlockSouthAlign(const BlockIndex& block, std::string name, const sf::Color* = nullptr);
	void progressBarOnBlock(const BlockIndex& block, Percent progress);

	void selected(const BlockIndex& block);
	void outlineOnBlock(const BlockIndex& block, const sf::Color color, float thickness = 3.f);
	void stringAtPosition(const std::wstring string, const sf::Vector2f position, const sf::Color color, float offsetX = 0.5, float offsetY = 0.0);
	void stringOnBlock(const BlockIndex& block,const  std::wstring string, const sf::Color color, float offsetX = 0.5, float offsetY = 0.0);

	void nonGroundCoverPlant(const BlockIndex& block);
	void item(const BlockIndex& block);
	void itemOverlay(const BlockIndex& block);
	void item(const ItemIndex& item, sf::Vector2f position);
	void itemOverlay(const ItemIndex& item, sf::Vector2f position);
	void singleTileActor(const ActorIndex& actor);
	void multiTileActor(const ActorIndex& actor);
	void actorOverlay(const ActorIndex& actor);
	void multiTileBorder(const OccupiedBlocksForHasShape& blocksOccpuied, sf::Color color, float thickness);
	void borderSegmentOnBlock(const BlockIndex& block, const Facing& facing, sf::Color color, float thickness);
	void accessableSymbol(const BlockIndex& block);
	void inaccessableSymbol(const BlockIndex& block);

	// Connects to an open top block, tries to align with an open bottom block.
	[[nodiscard]] Facing rampOrStairsFacing(const BlockIndex& block) const;
	[[nodiscard]] sf::Vector2f blockToPosition(const BlockIndex& block) const;
	[[nodiscard]] sf::Vector2f blockToPositionCentered(const BlockIndex& block) const;
	[[nodiscard]] float getScaledUnit() const;
};
