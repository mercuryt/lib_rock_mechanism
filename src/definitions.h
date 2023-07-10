#pragma once

#include "../lib/json.hpp"
#include "materialType.h"
#include "fluidType.h"
#include "body.h"
#include "plant.h"
#include "animalSpecies.h"
#include "item.h"
#include "moveType.h"

#include <fstream>
#include <filesystem>

using Json = nlohmann::json;
namespace definitions
{
	void loadShapes()
	{
		std::ifstream f("data/shapes.json");
		Json data = Json::parse(f);
		for(const Json& shapeData : data)
		{
			BodyPartType::data.emplace_back(
					shapeData["name"],
					shapeData["positions"]
					);
		}
	}
	void loadFluidTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/fluids"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			FluidType::data.emplace_back(
				data["name"],
				data["viscosity"],
				data["mistDuration"],
				data["mistSpread"]
			);
		}
	}
	void loadMoveTypes()
	{
		std::ifstream f("data/moveTypes.json");
		Json data = Json::parse(f);
		for(const Json& moveTypeData : data)
		{
			MoveType::data.emplace_back(
				moveTypeData["name"],
				moveTypeData["walk"],	
				moveTypeData["climb"],	
				moveTypeData["jumpDown"],	
				moveTypeData["fly"]
			);
			for(auto& pair : moveTypeData["swim"].items())
				MoveType::data.back().swim[&FluidType::byName(pair.key())] = pair.value();
		}
	}
	void loadBodyParts()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/bodyParts"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			BodyPartType& bodyPartType = BodyPartType::data.emplace_back(
				data["name"],
				data["volume"],
				data["doesLocamotion"],
				data["doesManipulation"]
			);
			for(const Json& attackTypeData : data["attackTypes"])
			{
				bodyPartType.attackTypes.emplace_back(
					attackTypeData["name"],
					attackTypeData["area"],
					attackTypeData["baseForce"],
					WoundType::byName(attackTypeData["woundType"])
				);
			}
		}
	}
	void loadItemTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			auto& itemType = ItemType::data.emplace_back(
				data["name"],
				data["installable"],
				Shape::byName(data["shape"]),
				data["generic"],
				data["internalVolume"],
				data["canHoldFluids"]
			);
			for(Json attackTypeData : data["attackTypes"])
			{
				itemType.attackTypes.emplace_back(
					attackTypeData["name"],
					attackTypeData["area"],
					attackTypeData["baseForce"],
					WoundType::byName(attackTypeData["woundType"])
				);
			}
			if(data["wearableType"])
			{
				Json& wearableTypeData = data["wearableType"];
				auto& wearableType = WearableType::data.emplace_back(
					wearableTypeData["percentCoverage"],
					wearableTypeData["defenseScore"],
					wearableTypeData["rigid"],
					wearableTypeData["impactSpreadArea"],
					wearableTypeData["forceAbsorbedPiercedModifier"]
				);
				for(const auto& bodyPartName : data["bodyPartsCovered"])
					wearableType.bodyPartsCovered.push_back(&BodyPartType::byName(bodyPartName));
				itemType.wearableType = &wearableType;
			}
		}
	}
	void loadMaterialTypes()
	{
		std::ifstream f("data/materialTypeCategories.json");
		for(const Json& data : Json::parse(f))
			MaterialTypeCategory::data.emplace_back(data["name"]);
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			auto& materialType = MaterialType::data.emplace_back(
				data["name"],
				MaterialTypeCategory::byName(data["category"]),
				data["density"],
				data["hardness"],
				data["transparent"],
				data["ignitionTemperature"],
				data["flameTemperature"],
				data["burnStageDuration"],
				data["flameStageDuration"]
			);
			for(Json& spoilData : data["spoilData"])
				materialType.spoilData.emplace_back(
					MaterialType::byName(spoilData["materialType"]),
					ItemType::byName(spoilData["itemType"]),
					spoilData["chance"],
					spoilData["min"],
					spoilData["max"]
				);
		}
	}
	void loadPlantSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			auto& plantSpecies = PlantSpecies::data.emplace_back(
				data["name"],
				data["annual"],
				data["maximumGrowingTemperature"],
				data["minimumGrowingTemperature"],
				data["stepsTillDieFromTemperature"],
				data["stepsNeedsFluidFrequency"],
				data["stepsTillDieWithoutFluid"],
				data["stepsTillFullyGrown"],
				data["growsInSunLight"],
				data["rootRangeMax"],
				data["rootRangeMin"],
				data["adultMass"],
				FluidType::byName(data["fluidType"])
			);
			if(data.contains("harvestData"))
			{
				auto& harvestData = HarvestData::data.emplace_back(
					data["harvestData"]["dayOfYearToStart"],
					data["harvestData"]["daysDuration"],
					data["harvestData"]["quantity"],
					ItemType::byName(data["harvestData"]["itemType"])
				);
				plantSpecies.harvestData = &harvestData;
			}
			else
				plantSpecies.harvestData = nullptr;
		}
	}
	void loadAnimalSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			auto& animalSpecies = AnimalSpecies::data.emplace_back(
				data["name"],
				data["sentient"],
				data["strength"],
				data["dextarity"],
				data["agility"],
				data["mass"],
				data["volume"],
				data["deathAge"],
				data["adultAge"],
				MoveType::byName(data["moveType"]),
				FluidType::byName(data["fluidType"])
			);
			for(const auto& shapeName : data["shapes"])
				animalSpecies.shapes.push_back(&Shape::byName(shapeName));
			for(const auto& bodyPartName : data["bodyParts"])
				animalSpecies.bodyPartTypes.push_back(&BodyPartType::byName(bodyPartName));
		}
	}
	void load()
	{
		loadShapes();
		loadFluidTypes();
		loadMoveTypes();
		loadBodyParts();
		loadItemTypes();
		loadMaterialTypes();
		loadPlantSpecies();
		loadAnimalSpecies();
	}
}
