#pragma once

#include "../lib/json.hpp"
#include "materialType.h"

#include <fstream>
#include <filesystem>

using Json = nlohmann::json;
namespace definitions
{
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
			std::ifstream f(file.path);
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
					moveTypeData["fly"],	
				);
				for(const std::pair& swimData : moveTypeData["swim"])
					MoveType::data.back().swim[FluidType::byName(pair.first)] = pair.second;
			}
	}
	void loadBodyParts()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/bodyParts"))
		{
			std::ifstream f(file.path);
			Json data = Json::parse(f);
			BodyPartType::data.emplace_back(
				data["name"],
				data["displayName"],
				data["volume"],
				data["doesLocamotion"],
				data["doesManipulation"]
			);
			for(const Json& attackTypeData : data["attackTypes"])
			{
				AttackType::data.emplace_back(
					attackTypeData["name"],
					attackTypeData["area"],
					attackTypeData["baseForce"],
					WoundType::byName(attackTypeData["woundType"]),
				);
				BodyPartType::data.back().attackTypes.push_back(&AttackType::data.back());
			}
		}
	}
	void loadItemTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path);
			Json data = Json::parse(f);
			ItemType::data.emplace_back(
				data["name"],
				data["installable"],
				Shape::byName(data["shape"]),
				data["generic"],
				data["internalVolume"],
				data["canHoldFluids"]
			);
			for(Json attackTypeData : data["attackTypes"])
			{
				AttackType::data.emplace_back(
					attackTypeData["name"],
					attackTypeData["area"],
					attackTypeData["baseForce"],
					WoundType.byName(attackTypeData["WoundType"])
				);
				ItemType::data.back().attackTypes.push_back(&AttackType::data.back());
			}
			if(data["wearableType"])
			{
				Json& wearableTypeData = data["wearableType"];
				WearableType::data.emplace_back(
					wearableTypeData["percentCoverage"],
					wearableTypeData["defenseScore"],
					wearableTypeData["rigid"],
					wearableTypeData["impactSpreadArea"],
					wearableTypeData["forceAbsorbedPiercedModifier"],

				);
				for(std::wstring bodyPartName : data["bodyPartsCovered"])
					WearableType::data.back().bodyPartsCovered.push_back(&BodyPartType::byName(bodyPartName));
			}
		}
	}
	void loadMaterialTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path);
			Json data = Json::parse(f);
			MaterialType::data.emplace_back(
				data["name"],
				data["density"],
				data["hardness"],
				data["transparent"],
				data["ignitionTemperature"],
				data["flameTemperature"],
				data["burnStageDuration"],
				data["flameStageDuration"]
			);
			for(Json& spoilData : data["spoilData"])
				MaterialType::data.back().spoilData.emplace_back(
					MaterialType.byName(spoilData["materialType"]),
					ItemType.byName(spoilData["itemType"]),
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
			std::ifstream f(file.path);
			Json data = Json::parse(f);
			PlantSpecies::data.emplace_back(
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
			if(data.contains["harvestData"])
			{
				HarvestData::data.emplace_back(
					data["harvestData"]["dayOfYearToStart"],
					data["harvestData"]["daysDuration"],
					data["harvestData"]["quantity"],
					ItemType::byName(data["harvestData"]["itemType"])
				);
				PlantSpecies::data.back().harvestData = HarvestData::data.back();
			}
		}
	}
	void loadAnimalSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/materials"))
		{
			std::ifstream f(file.path);
			Json data = Json::parse(f);
			AnimalSpecies::data.emplace_back(
				data["name"],
				data["sentient"],
				data["strength"],
				data["dextarity"],
				data["agility"],
				data["mass"],
				data["volume"],
				data["deathAge"],
				data["adultSizeAge"],
				MoveType::byName(data["moveType"]),
				Shape::byName(data["shape"]),
				FluidType::byName(data["fluidType"])
			);
			for(const std::wstring& bodyPartName : data["bodyParts"])
				AnimalSpecies::data.back().bodyParts.push_back(&BodyPart::byName(bodyPartName));
		}
	}
};
