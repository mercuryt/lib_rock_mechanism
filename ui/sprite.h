#pragma once
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <cassert>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
namespace sprites
{
	inline const std::filesystem::path path = "img/build/sheet0.png";
	inline std::unordered_map<std::string, sf::Texture> textures;
	inline std::vector<sf::Sprite> sprites;
	void load();
	void flush();
	sf::Sprite& make(std::string name);
};
