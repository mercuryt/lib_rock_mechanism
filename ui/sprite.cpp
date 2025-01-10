#include "sprite.h"
#include "../engine/definitions.h"
#include "../img/spright.h"
#include <SFML/Graphics/Rect.hpp>
void sprites::load()
{
	spriteCoordinates::load();
	for(auto& [name, spriteCoords] : spriteCoordinates::data)
	{
		sf::Texture texture;
		textures[name].first.loadFromFile(path, sf::IntRect(spriteCoords.x, spriteCoords.y, spriteCoords.w, spriteCoords.h));
		textures[name].second = {spriteCoords.pivot_x, spriteCoords.pivot_y};
	}
}
void sprites::flush() { sprites.clear(); }
std::pair<sf::Sprite, sf::Vector2f> sprites::make(std::wstring name)
{
	sprites.emplace_back();
	sprites.back().setTexture(textures[name].first);
	return {sprites.back(), textures[name].second};
}
