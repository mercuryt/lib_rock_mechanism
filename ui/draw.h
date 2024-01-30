#pragma once
#include <SFML/Graphics/Color.hpp>
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
	void block(const Block& block);
	void validOnBlock(const Block& block);
	void invalidOnBlock(const Block& block);
	void colorOnBlock(const Block& block, sf::Color color);
	void imageOnBlock(const Block& block, std::string name, sf::Color* = nullptr);
	void outlineOnBlock(const Block& block, sf::Color color, float thickness = 3.f);
	void stringOnBlock(const Block& block, std::wstring string, sf::Color color);

	void item(const Item& item);
	void actor(const Actor& actor);
	void plant(const Plant& plant);
};
