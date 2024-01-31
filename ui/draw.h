#pragma once
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
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
	void validOnBlock(const Block& block);
	void invalidOnBlock(const Block& block);
	void colorOnBlock(const Block& block, sf::Color color);

	void spriteOnBlock(const Block& block, sf::Sprite& sprite, sf::Color* = nullptr);
	void imageOnBlock(const Block& block, std::string name, sf::Color* = nullptr);
	void imageOnBlockWestAlign(const Block& block, std::string name, sf::Color* = nullptr);
	void imageOnBlockEastAlign(const Block& block, std::string name, sf::Color* = nullptr);
	void imageOnBlockSouthAlign(const Block& block, std::string name, sf::Color* = nullptr);

	void outlineOnBlock(const Block& block, sf::Color color, float thickness = 3.f);
	void stringOnBlock(const Block& block, std::wstring string, sf::Color color);

	void item(const Item& item);
	void actor(const Actor& actor);
	void plant(const Plant& plant);
};
