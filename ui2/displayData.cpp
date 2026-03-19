#include "displayData.h"
#include "../engine/dataStructures/smallMap.h"
#include "../engine/definitions/definitions.h"
#include "../engine/definitions/materialType.h"
#include "../engine/definitions/animalSpecies.h"
#include "../engine/definitions/plantSpecies.h"
#include "../engine/fluidType.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include "../engine/pointFeature.h"
#include <SFML/Graphics/Color.hpp>
#include <string>

SDL_Color colorFromJson(const Json& data)
{
	auto r = data[0].get<uint8_t>();
	auto g = data[1].get<uint8_t>();
	auto b = data[2].get<uint8_t>();
	auto a = data[3].get<uint8_t>();
	return SDL_Color(r, g, b, a);
}
std::string displayData::localizeNumber(double number)
{
	static std::locale cpploc{""};
	std::stringstream ss;
	ss.imbue(cpploc);
	ss << number;
	return ss.str();
}
std::string displayData::formatTemperature(Temperature temperature)
{
	switch(temperatureDisplayUnits)
	{
		case TemperatureDisplayUnits::K:
			return temperature.toString() + "K";
		case TemperatureDisplayUnits::C:
			return std::to_string((int)temperature.get() - 273) + "C";
		case TemperatureDisplayUnits::F:
			return std::to_string((int)(((float)temperature.get() - 273.15f) * (9.0f / 5.0f)) + 32) + "F";
		default:
			std::unreachable();
	}
}
void displayData::load()
{
	std::filesystem::path path = definitions::path/"display";
	for(const Json& data : definitions::tryParse(path/"materialTypes.json"))
	{
		const MaterialTypeId& materialType = MaterialType::byName(data["name"].get<std::string>());
		materialColors.insert(materialType, colorFromJson(data["color"]));
	}
	for(const Json& data : definitions::tryParse(path/"fluidTypes.json"))
	{
		const FluidTypeId& fluidType = FluidType::byName(data["name"].get<std::string>());
		fluidColors.insert(fluidType, colorFromJson(data["color"]));
	}
	for(const Json& data : definitions::tryParse(path/"itemTypes.json"))
	{
		const ItemTypeId& itemType = ItemType::byName(data["name"].get<std::string>());
		std::string image = data["image"].get<std::string>();
		itemData.emplace(
			itemType,
			image,
			Sprite(image),
			data.contains("color") ? colorFromJson(data["color"]) : SDL_Color{255,255,255,255},
			data.contains("scale") ? data["scale"].get<float>() : 1.0f
		);
	}
	for(const Json& data : definitions::tryParse(path/"plantSpecies.json"))
	{
		const PlantSpeciesId& plantSpecies = PlantSpecies::byName(data["name"].get<std::string>());
		std::string image = data["image"].get<std::string>();
		plantData.emplace(
			plantSpecies,
			image,
			Sprite(image),
			data.contains("color") ? colorFromJson(data["color"]) : SDL_Color{255,255,255,255},
			data.contains("scale") ? data["scale"].get<float>() : 1.0f,
			data.contains("groundCover") && data["groundCover"].get<bool>()
		);
	}
	for(const Json& data : definitions::tryParse(path/"animalSpecies.json"))
	{
		const AnimalSpeciesId& animalSpecies = AnimalSpecies::byName(data["name"].get<std::string>());
		std::string image = data["image"].get<std::string>();
		actorData.emplace(animalSpecies,
			image,
			Sprite(image),
			data.contains("color") ? colorFromJson(data["color"]) : SDL_Color{255,255,255,255},
			data.contains("scale") ? data["scale"].get<float>() : 1.0f
		);
	}
	const Json& data = definitions::tryParse(path/"config.json");
	ratioOfScaleToFontSize = data["ratioOfScaleToFontSize"].get<float>();
	wallOffset = data["wallOffset"].get<float>();
	maximumNumberOfItemsToDisplayInComboBox = data["maximumNumberOfItemsToDisplayInComboBox"].get<std::size_t>();
	data["temperatureDisplayUnits"].get_to(temperatureDisplayUnits);
}
