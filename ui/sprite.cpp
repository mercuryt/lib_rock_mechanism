#include "sprite.h"
#include "../engine/definitions/definitions.h"
#include "../img/spright.h"
#include <SFML/Graphics/Rect.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
void sprites::load()
{
	spriteCoordinates::load();
	for(auto& [name, spriteCoords] : spriteCoordinates::data)
	{
		textures.insert(name, {{}, {spriteCoords.pivot_x, spriteCoords.pivot_y}});
		sf::Texture& texture = textures.back().second.first;
		// All textures are set repeated.
		texture.setRepeated(true);
		texture.loadFromFile(path, sf::IntRect(spriteCoords.x, spriteCoords.y, spriteCoords.w, spriteCoords.h));
	}
}
std::pair<sf::Sprite, sf::Vector2f> sprites::make(const std::string& name)
{
	sf::Sprite output;
	auto& [texture, origin] = textures[name];
	output.setTexture(texture);
	return {output, origin};
}
sf::Texture sprites::makeRotatedTexture(const std::string& name, const int& rotation)
{
	auto& [texture, origin] = textures[name];
	sf::Sprite rotated(texture);
	const auto& size = texture.getSize();
	rotated.setOrigin(size.x / 2.f, size.y / 2.f);
	rotated.setPosition(size.x / 2.f, size.y / 2.f);
	rotated.setRotation(rotation);
	sf::RenderTexture renderTexture;
	renderTexture.create(size.x, size.y);
	renderTexture.clear(sf::Color::Transparent);
	renderTexture.draw(rotated);
	renderTexture.display();
	sf::Texture result;
	result.loadFromImage(renderTexture.getTexture().copyToImage());
	result.setRepeated(true);
	return result;
}
sf::Sprite sprites::makeRepeated(const std::string& name, const Cuboid& cuboid)
{
	auto& [texture, origin] = textures[name];
	return makeRepeated(texture, cuboid);
}
sf::Sprite sprites::makeRepeated(const sf::Texture& texture, const Cuboid& cuboid)
{
	assert(texture.isRepeated());
	sf::Sprite output;
	output.setTexture(texture);
	auto [sizeX, sizeY] = texture.getSize();
	sf::IntRect rect{0, 0, (int)sizeX * cuboid.sizeX().get(), (int)sizeY * cuboid.sizeY().get()};
	output.setTextureRect(rect);
	return output;
}
sf::Sprite sprites::makeRepeatedRotated(const std::string& name, const Cuboid& cuboid, const int& rotation)
{
	sf::Sprite output;
	auto& [texture, origin] = textures[name];
	texture.setRepeated(true);
	output.setTexture(texture);
	auto [sizeX, sizeY] = texture.getSize();
	sf::IntRect rect{(int)origin.x, (int)origin.y, (int)sizeX * cuboid.sizeX().get(), (int)sizeY * cuboid.sizeY().get()};
	output.setTextureRect(rect);
	// Center origin so rotation behaves nicely
	output.setOrigin( rect.width / 2.f, rect.height / 2.f);
	output.rotate(rotation);
	return output;
}