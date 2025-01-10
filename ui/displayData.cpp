#include "displayData.h"
#include "../engine/definitions.h"
#include "../engine/materialType.h"
#include "../engine/fluidType.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include "../engine/animalSpecies.h"
#include "../engine/blockFeature.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

std::wstring loadWString(const Json& data)
{
	std::wstring output;
	if(data.is_string())
		for(auto codePoint : data.get<std::wstring>())
			output += codePoint;
	else
		for(const Json& codePoint : data)
			output += std::to_wstring(codePoint.get<long>());
	return output;
}

sf::Color colorFromJson(const Json& data)
{
	auto r = data[0].get<uint8_t>();
	auto g = data[1].get<uint8_t>();
	auto b = data[2].get<uint8_t>();
	auto a = data[3].get<uint8_t>();
	return sf::Color(r, g, b, a);
}
std::wstring displayData::localizeNumber(double number)
{
	static std::locale cpploc{""};
	std::wstringstream ss;
	ss.imbue(cpploc);
	ss << number;
	return ss.str();
}
void displayData::load()
{
	std::filesystem::path path = definitions::path/"display";
	for(const Json& data : definitions::tryParse(path/"materialTypes.json"))
	{
		const MaterialTypeId& materialType = MaterialType::byName(data["name"].get<std::wstring>());
		materialColors[&materialType] = colorFromJson(data["color"]);
	}
	for(const Json& data : definitions::tryParse(path/"fluidTypes.json"))
	{
		const FluidTypeId& fluidType = FluidType::byName(data["name"].get<std::wstring>());
		fluidColors[&fluidType] = colorFromJson(data["color"]);
	}
	for(const Json& data : definitions::tryParse(path/"itemTypes.json"))
	{
		const ItemTypeId& itemType = ItemType::byName(data["name"].get<std::wstring>());
		[[maybe_unused]] auto pair = itemData.try_emplace( &itemType,
			data["image"].get<std::wstring>(),
			data.contains("color") ? colorFromJson(data["color"]) : sf::Color::White,
			data.contains("scale") ? data["scale"].get<float>() : 1.0f
		);
		assert(pair.second);
	}
	for(const Json& data : definitions::tryParse(path/"plantSpecies.json"))
	{
		const PlantSpeciesId& plantSpecies = PlantSpecies::byName(data["name"].get<std::wstring>());
		[[maybe_unused]] auto pair = plantData.try_emplace( &plantSpecies,
			data["image"].get<std::wstring>(),
			data.contains("color") ? colorFromJson(data["color"]) : sf::Color::White,
			data.contains("scale") ? data["scale"].get<float>() : 1.0f,
			data.contains("groundCover") && data["groundCover"].get<bool>()
		);
		assert(pair.second);
	}
	for(const Json& data : definitions::tryParse(path/"animalSpecies.json"))
	{
		const AnimalSpeciesId& animalSpecies = AnimalSpecies::byName(data["name"].get<std::wstring>());
		[[maybe_unused]] auto pair = actorData.try_emplace( &animalSpecies,
			data["image"].get<std::wstring>(),
			data.contains("color") ? colorFromJson(data["color"]) : sf::Color::White,
			data.contains("scale") ? data["scale"].get<float>() : 1.0f
		);
		assert(pair.second);
	}
	const Json& data = definitions::tryParse(path/"config.json");
	ratioOfScaleToFontSize = data["ratioOfScaleToFontSize"].get<float>();
	wallOffset = data["wallOffset"].get<float>();
	progressBarColor = sf::Color::Blue;
	progressBarOutlineColor = sf::Color::Black;
	progressBarThickness = 2.f;
	maximumNumberOfItemsToDisplayInComboBox = data["maximumNumberOfItemsToDisplayInComboBox"].get<std::size_t>();
}
