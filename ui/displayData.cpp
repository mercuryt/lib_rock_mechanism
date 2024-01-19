#include "displayData.h"
#include "../engine/definitions.h"
#include "../engine/materialType.h"
#include "../engine/fluidType.h"
#include "../engine/item.h"
#include "../engine/plant.h"
#include "../engine/animalSpecies.h"
#include "../engine/blockFeature.h"
#include <SFML/Graphics/Color.hpp>

sf::Color colorFromJson(const Json& data)
{
	auto r = data[0].get<uint8_t>();
	auto b = data[1].get<uint8_t>();
	auto g = data[2].get<uint8_t>();
	auto a = data[3].get<uint8_t>();
	return sf::Color(r, g, b, a);
}
void displayData::load()
{
	std::filesystem::path path = definitions::path/"display";
	for(const Json& data : definitions::tryParse(path/"materialTypes.json"))
	{
		const MaterialType& materialType = MaterialType::byName(data["name"].get<std::string>());
		materialColors[&materialType] = colorFromJson(data["color"]);
	}
	for(const Json& data : definitions::tryParse(path/"fluidTypes.json"))
	{
		const FluidType& fluidType = FluidType::byName(data["name"].get<std::string>());
		fluidColors[&fluidType] = colorFromJson(data["color"]);
	}
	for(const Json& data : definitions::tryParse(path/"itemTypes.json"))
	{
		const ItemType& itemType = ItemType::byName(data["name"].get<std::string>());
		itemSymbols[&itemType] = data["symbol"].get<std::wstring>();
	}
	for(const Json& data : definitions::tryParse(path/"plantSpecies.json"))
	{
		const PlantSpecies& plantSpecies = PlantSpecies::byName(data["name"].get<std::string>());
		plantSymbols[&plantSpecies] = data["symbol"].get<std::wstring>();
	}
	for(const Json& data : definitions::tryParse(path/"actorSpecies.json"))
	{
		const AnimalSpecies& animalSpecies = AnimalSpecies::byName(data["name"].get<std::string>());
		actorSymbols[&animalSpecies] = data["symbol"].get<std::wstring>();
	}
	for(const Json& data : definitions::tryParse(path/"blockFeature.json"))
	{
		const BlockFeatureType& blockFeatureType = BlockFeatureType::byName(data["name"].get<std::string>());
		blockFeatureSymbols[&blockFeatureType] = data["symbol"].get<std::wstring>();
	}
}
