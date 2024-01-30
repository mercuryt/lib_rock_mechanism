#include "displayData.h"
#include "../engine/definitions.h"
#include "../engine/materialType.h"
#include "../engine/fluidType.h"
#include "../engine/item.h"
#include "../engine/plant.h"
#include "../engine/animalSpecies.h"
#include "../engine/blockFeature.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

std::wstring loadWString(const Json& data)
{
	std::wstring output;
	if(data.is_string())
		for(auto codePoint : data.get<std::string>())
			output += codePoint;
	else
		for(const Json& codePoint : data)
			output += std::to_wstring(codePoint.get<long>());
	return output;
}

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
		itemSymbols[&itemType] = loadWString(data["symbol"]);
	}
	for(const Json& data : definitions::tryParse(path/"plantSpecies.json"))
	{
		const PlantSpecies& plantSpecies = PlantSpecies::byName(data["name"].get<std::string>());
		plantSymbols[&plantSpecies] = loadWString(data["symbol"]);
	}
	for(const Json& data : definitions::tryParse(path/"animalSpecies.json"))
	{
		const AnimalSpecies& animalSpecies = AnimalSpecies::byName(data["name"].get<std::string>());
		actorSymbols[&animalSpecies] = loadWString(data["symbol"]);
		actorColors[&animalSpecies] = colorFromJson(data["color"]);
	}
	for(const Json& data : definitions::tryParse(path/"blockFeatures.json"))
	{
		const BlockFeatureType& blockFeatureType = BlockFeatureType::byName(data["name"].get<std::string>());
		blockFeatureSymbols[&blockFeatureType] = loadWString(data["symbol"]);
	}
	const Json& data = definitions::tryParse(path/"config.json");
	ratioOfScaleToFontSize = data["ratioOfScaleToFontSize"].get<float>();
}
