#pragma once

#include "../engine/dataStructures/smallMap.h"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/System/Vector2.hpp>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>
class Cuboid;
namespace sprites
{
	inline const std::filesystem::path path = "img/build/sheet0.png";
	inline SmallMap<std::string, std::pair<sf::Texture, sf::Vector2f>> textures;
	void load();
	std::pair<sf::Sprite, sf::Vector2f> make(const std::string& name);
	sf::Sprite makeRepeated(const std::string& name, const Cuboid& cuboid);
	sf::Sprite makeRepeated(const sf::Texture& texture, const Cuboid& cuboid);
	sf::Texture makeRotatedTexture(const std::string& name, const int& rotation);
	// Return rotated texture to ensure it stays in scope as long as sprite does.
	sf::Sprite makeRepeatedRotated(const std::string& name, const Cuboid& cuboid, const int& rotation);
};
