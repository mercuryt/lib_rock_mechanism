#pragma once
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>
namespace sprites
{
	inline const std::filesystem::path path = "img/build/sheet0.png";
	inline SmallMap<std::wstring, std::pair<sf::Texture, sf::Vector2f>> textures;
	inline std::vector<sf::Sprite> sprites;
	void load();
	void flush();
	std::pair<sf::Sprite, sf::Vector2f> make(std::wstring name);
};
