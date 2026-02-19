#pragma once

#include "../engine/dataStructures/smallMap.h"
#include "imgui/imgui.h"

#include <SDL2/SDL.h>
#include <cassert>
#include <filesystem>
#include <string>
#include <vector>
class Cuboid;
namespace sprites
{
	inline const std::filesystem::path path = "img/build/sheet0.png";
	inline SmallMap<std::string, std::pair<SDL_Texture, ImVec2>> textures;
	void load();
	std::pair<sf::Sprite, ImVec2> make(const std::string& name);
	sf::Sprite makeRepeated(const std::string& name, const Cuboid& cuboid);
	sf::Sprite makeRepeated(const SDL_Texture& texture, const Cuboid& cuboid);
	SDL_Texture makeRotatedTexture(const std::string& name, const int& rotation);
	// Return rotated texture to ensure it stays in scope as long as sprite does.
	sf::Sprite makeRepeatedRotated(const std::string& name, const Cuboid& cuboid, const int& rotation);
};
