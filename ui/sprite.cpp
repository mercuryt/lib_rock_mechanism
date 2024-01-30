#include "sprite.h"
#include "../engine/definitions.h"
#include "../img/spright.h"
#include "definitions.h"
#include <SFML/Graphics/Rect.hpp>
void sprites::load()
{
	spriteCoordinates::load();
	for(auto& [name, spriteCoords] : spriteCoordinates::data)
	{
		sf::Texture texture;
		textures[name].loadFromFile(path, sf::IntRect(spriteCoords.x, spriteCoords.y, spriteCoords.w, spriteCoords.h));
	}
}
void sprites::flush() { sprites.clear(); }
sf::Sprite& sprites::make(std::string name)
{
	sprites.emplace_back();
	sprites.back().setTexture(textures[name]);
	return sprites.back();
}
