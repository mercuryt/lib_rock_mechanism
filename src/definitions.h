#pragma once

#include "../lib/json.hpp"
#include "materialType.h"
#include "fluidType.h"
#include "body.h"
#include "plant.h"
#include "animalSpecies.h"
#include "item.h"
#include "moveType.h"
#include "attackType.h"
#include "skill.h"

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
	void loadSkillTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/items"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			SkillType::data.emplace_back(
				data["name"],
				data["xpPerLevelModifier"],
				data["level1Xp"]
			);
		}
	}
	const AttackType loadAttackType(const Json& data)
	{
		return AttackType(
			data["name"],
			data["area"],
			data["baseForce"],
			data["range"],
			data["skillBonusOrPenalty"],
			WoundType::byName(data["woundType"]),
			SkillType::byName(data["skillType"])
		);
	}
	void loadItemTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/items"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			auto& itemType = ItemType::data.emplace_back(
				data["name"],
				data["installable"],
				Shape::byName(data["shape"]),
				data["volume"],
				data["generic"],
				data["internalVolume"],
				data["canHoldFluids"],
				data["combatScoreBonus"],
				FluidType::byName(data["edibleForDrinkersOf"]),
				MoveType::byName(data["moveType"])
			);
			if(data["combatSkill"])
				itemType.combatSkill = &SkillType::byName(data["combatSkill"]);
			if(data["wearableType"])
			{
				Json& wearableTypeData = data["wearableType"];
				auto& wearableType = WearableType::data.emplace_back(
					wearableTypeData["name"],
					wearableTypeData["percentCoverage"],
					wearableTypeData["defenseScore"],
					wearableTypeData["rigid"],
					wearableTypeData["impactSpreadArea"],
					wearableTypeData["forceAbsorbedPiercedModifier"],
					wearableTypeData["forceAbsorbedUnpiercedModifier"],
					wearableTypeData["layer"],
					BodyTypeCategory::byName(wearableTypeData["bodyTypeCategory"]),
					wearableTypeData["bodyTypeScale"]
				);
				for(const auto& bodyPartName : data["bodyPartsCovered"])
					wearableType.bodyPartsCovered.push_back(&BodyPartType::byName(bodyPartName));
				itemType.wearableType = &wearableType;
			}
			for(Json attackTypeData : data["attackTypes"])
				itemType.attackTypes.push_back(loadAttackType(attackTypeData));
		}
	}
	void loadPlantSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/plants"))
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
				data["volumeFluidConsumed"],
				data["stepsTillDieWithoutFluid"],
				data["stepsTillFullyGrown"],
			 	data["stepsTillFoliageGrowsFromZero"],
				data["growsInSunLight"],
				data["rootRangeMax"],
				data["rootRangeMin"],
				data["adultMass"],
				data["dayOfYearForSowStart"],
				data["dayOfYearForSowEnd"],
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
	void loadBodyPartTypes()
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
			for(const Json& pair : data["attackTypesAndMaterials"])
				bodyPartType.attackTypesAndMaterials.emplace_back(loadAttackType(pair.at(0)), &MaterialType::byName(pair.at(1)));
		}
	}
	void loadBodyTypeCategories()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/bodyCategories"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			BodyTypeCategory::data.emplace_back(
				data["name"]
			);
		}
	}
	void loadBodyTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/bodies"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			BodyType& bodyType = BodyType::data.emplace_back(
				data["name"],
				BodyTypeCategory::byName(data["category"]),
				data["scale"]
			);
			for(const Json& bodyPartTypeName : data["bodyPartTypes"])
				bodyType.bodyPartTypes.push_back(&BodyPartType::byName(bodyPartTypeName));
		}
	}
	void loadAnimalSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator("data/animals"))
		{
			std::ifstream f(file.path());
			Json data = Json::parse(f);
			AnimalSpecies& animalSpecies = AnimalSpecies::data.emplace_back(
				data["name"],
				data["sentient"],
				data["strength"],
				data["dextarity"],
				data["agility"],
				data["mass"],
				data["volume"],
				data["deathAge"],
				data["adultAge"],
				data["stepsTillDieWithoutFood"],
				data["stepsEatFrequency"],
				data["stepsTillDieWithoutFluid"],
				data["stepsFluidDrinkFreqency"],
				data["stepsTillFullyGrown"],
				data["stepsTillDieInUnsafeTemperature"],
				data["stepsSleepFrequency"],
				data["stepsTillSleepOveride"],
				data["stepsSleepDuration"],
				data["minimumSafeTemperature"],
				data["maximumSafeTemperature"],
				data["eatsMeat"],
				data["eatsLeaves"],
				data["eatsFruit"],
				MaterialType::byName(data["materialType"]),
				MoveType::byName(data["moveType"]),
				FluidType::byName(data["fluidType"]),
				BodyType::byName(data["bodyType"])
			);
			for(const auto& shapeName : data["shapes"])
				animalSpecies.shapes.push_back(&Shape::byName(shapeName));
		}
	}
	void load()
	{
		loadShapes();
		loadFluidTypes();
		loadMaterialTypes();
		loadMoveTypes();
		loadSkillTypes();
		loadItemTypes();
		loadPlantSpecies();
		loadBodyPartTypes();
		loadBodyTypeCategories();
		loadBodyTypes();
		loadAnimalSpecies();
	}
}
