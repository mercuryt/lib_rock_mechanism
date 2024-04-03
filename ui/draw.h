#pragma once
#include "../engine/types.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
#include <string>
class Window;
class Block;
class Item;
class Actor;
class Plant;
class Draw final
{
	Window& m_window;
public:
	Draw(Window& w) : m_window(w) { }
	void blockFloor(const Block& block);
	void blockWallCorners(const Block& block);
	void blockWalls(const Block& block);
	void blockWallTops(const Block& block);
	void blockFeaturesAndFluids(const Block& block);
	void blockWallsFromNextLevelDown(const Block& block);
	void validOnBlock(const Block& block);
	void invalidOnBlock(const Block& block);
	void colorOnBlock(const Block& block, const sf::Color color);
	void designated(const Block& block);
	void craftLocation(const Block& block);
	sf::Sprite getCenteredSprite(std::string name);

	void spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color);
	void spriteOnBlockWithScaleCentered(const Block& block, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlockWithScale(const Block& block, sf::Sprite& sprite, float scaleRatio, const sf::Color* = nullptr);
	void spriteOnBlock(const Block& block, sf::Sprite& sprite, const sf::Color* = nullptr);
	void spriteOnBlockCentered(const Block& block, sf::Sprite& sprite, const sf::Color* = nullptr);
	void imageOnBlock(const Block& block, std::string name, const sf::Color* = nullptr);
	void imageOnBlockWestAlign(const Block& block, std::string name, const sf::Color* = nullptr);
	void imageOnBlockEastAlign(const Block& block, std::string name, const sf::Color* = nullptr);
	void imageOnBlockSouthAlign(const Block& block, std::string name, const sf::Color* = nullptr);
	void progressBarOnBlock(const Block& block, Percent progress);

	void selected(Block& block);
	void outlineOnBlock(const Block& block, const sf::Color color, float thickness = 3.f);
	void stringOnBlock(const Block& block, std::wstring string, const sf::Color color);

	void nonGroundCoverPlant(const Block& block);
	void item(const Block& block);
	void singleTileActor(const Actor& actor);
	void multiTileActor(const Actor& actor);
	void borderSegmentOnBlock(const Block& block, Facing facing, sf::Color color, float thickness);

	// Connects to an open top block, tries to align with an open bottom block.
	[[nodiscard]] Facing rampOrStairsFacing(const Block& block) const;
};
