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
#include "config.h"

#include <fstream>
#include <filesystem>
#include <algorithm>

using Json = nlohmann::json;
namespace definitions
{
	inline std::filesystem::path path = "data";
	inline Json tryParse(std::string filePath)
	{
		std::ifstream f(filePath);
		return Json::parse(f);
	}
	inline void loadShapes()
	{
		Json data = tryParse(path/"shapes.json");
		for(const Json& shapeData : data)
		{
			Shape::data.emplace_back(
				shapeData["name"].get<std::string>(),
				shapeData["positions"].get<std::vector<std::array<int32_t, 4>>>()
			);
		}
	}
	inline void loadFluidTypes()
	{
		assert(std::filesystem::exists(path/"fluids"));
		// Don't do freezesInto here because we haven't loaded soid materials yet.
		for(const auto& file : std::filesystem::directory_iterator(path/"fluids"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			FluidType::data.emplace_back(
				data["name"].get<std::string>(),
				data["viscosity"].get<std::uint32_t>(),
				data["density"].get<std::uint32_t>(),
				data.contains("mistDuration") ? data["mistDuration"].get<Step>() : 0,
				data.contains("mistDuration") ? data["maxMistSpread"].get<std::uint32_t>() : 0
			);
		}
	}
	inline void loadMoveTypes()
	{
		Json data = tryParse(path/"moveTypes.json");
		for(const Json& moveTypeData : data)
		{
			MoveType::data.emplace_back(
				moveTypeData["name"].get<std::string>(),
				moveTypeData["walk"].get<bool>(),
				moveTypeData["climb"].get<uint32_t>(),
				moveTypeData["jumpDown"].get<bool>(),
				moveTypeData["fly"].get<bool>()
			);
			for(auto& pair : moveTypeData["swim"].items())
				MoveType::data.back().swim[&FluidType::byName(pair.key())] = pair.value();
		}
	}
	inline void loadMaterialTypes()
	{
		for(const Json& data : tryParse(path/"materialTypeCategories.json"))
			MaterialTypeCategory::data.emplace_back(data["name"]);
		for(const auto& file : std::filesystem::directory_iterator(path/"materials"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto& materialType = MaterialType::data.emplace_back(
				data["name"].get<std::string>(),
				data.contains("category") ? 
					&MaterialTypeCategory::byName(data["category"].get<std::string>()) :
					nullptr,
				data["density"].get<uint32_t>(),
				data["hardness"].get<uint32_t>(),
				data["transparent"].get<bool>(),
				data.contains("meltingPoint") ? data["meltingPoint"].get<uint32_t>() : 0
			);
			if(data.contains("burnData"))
				materialType.burnData = &BurnData::data.emplace_back(
						data["burnData"]["ignitionTemperature"].get<Temperature>(),
						data["burnData"]["flameTemperature"].get<Temperature>(),
						data["burnData"]["burnStageDuration"].get<Step>(),
						data["burnData"]["flameStageDuration"].get<Step>()
				);
			if(data.contains("meltsInto"))
			{
				assert(materialType.meltingPoint != 0);
				materialType.meltsInto = &FluidType::byName(data["meltsInto"].get<std::string>());
			}
			else
			{
				assert(materialType.meltingPoint == 0);
				materialType.meltsInto = nullptr;
			}
		}
		// Load FluidType freeze into here, now that solid material types are loaded.
		for(const auto& file : std::filesystem::directory_iterator(path/"fluids"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			FluidType::byNameNonConst(data["name"].get<std::string>()).freezesInto = data.contains("freezesInto") ? 
				&MaterialType::byName(data["freezesInto"].get<std::string>()) :
				nullptr;

		}
	}
	inline void loadSkillTypes()
	{
		for(const Json& skillData : tryParse(path/"skills.json"))
		{
			SkillType::data.emplace_back(
				skillData["name"].get<std::string>(),
				skillData["xpPerLevelModifier"].get<float>(),
				skillData["level1Xp"].get<uint32_t>()
			);
		}
	}
	inline const AttackType loadAttackType(const Json& data)
	{
		return AttackType(
			data["name"].get<std::string>(),
			data["area"].get<uint32_t>(),
			data["baseForce"].get<Force>(),
			data["range"].get<uint32_t>(),
			data["skillBonus"].get<uint32_t>(),
			WoundCalculations::byName(data["woundType"].get<std::string>())
		);
	}
	inline void loadItemTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator(path/"items"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			const Shape& shape = Shape::byName(data["shape"].get<std::string>());
			uint32_t volume = std::accumulate(shape.positions.begin(), shape.positions.end(), 0, [](uint32_t total, auto& position) { return total + position[3]; });
			auto& itemType = ItemType::data.emplace_back(
				data["name"].get<std::string>(),
				data.contains("installable") && data["installable"].get<bool>() == true,
				shape,
				volume,
				data["generic"].get<bool>(),
				data.contains("internalVolume") ? data["internalVolume"].get<Volume>() : 0 ,
				data.contains("canHoldFluids") ? data["canHoldFluids"].get<bool>() : false,
				data["value"].get<uint32_t>(),
				data.contains("edibleForDrinkersOf") ? &FluidType::byName(data["edibleForDrinkersOf"].get<std::string>()) : nullptr,
				MoveType::byName(data["moveType"].get<std::string>())
			);
			if(data.contains("weaponData"))
			{
				Json& weaponData = data["weaponData"];
				auto& weapon = WeaponData::data.emplace_back(
					&SkillType::byName(weaponData["combatSkill"].get<std::string>()),
					weaponData["combatScoreBonus"].get<uint32_t>()
				);
				for(Json attackTypeData : weaponData["attackTypes"])
					weapon.attackTypes.push_back(loadAttackType(attackTypeData));
				itemType.weaponData = &weapon;
			}
			else
				itemType.weaponData = nullptr;
			if(data.contains("wearableData"))
			{
				Json& wearable = data["wearableData"];
				auto& wearableData = WearableData::data.emplace_back(
					wearable["percentCoverage"].get<Percent>(),
					wearable["defenseScore"].get<uint32_t>(),
					wearable["rigid"].get<bool>(),
					wearable["layer"].get<uint32_t>(),
					wearable["forceAbsorbedUnpiercedModifier"].get<uint32_t>(),
					wearable["forceAbsorbedPiercedModifier"].get<uint32_t>(),
					Config::scaleOfHumanBody
				);
				for(const auto& bodyPartName : data["bodyPartsCovered"])
					wearableData.bodyPartsCovered.push_back(&BodyPartType::byName(bodyPartName.get<std::string>()));
				itemType.wearableData = &wearableData;
			}
			else
				itemType.wearableData = nullptr;
		}
		// Now that item types are loaded we can load material type spoil data.
		for(const auto& file : std::filesystem::directory_iterator(path/"materials"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			MaterialType& materialType = MaterialType::byNameNonConst(data["name"].get<std::string>());
			for(Json& spoilData : data["spoilData"])
				materialType.spoilData.emplace_back(
					MaterialType::byName(spoilData["materialType"].get<std::string>()),
					ItemType::byName(spoilData["itemType"].get<std::string>()),
					spoilData["chance"].get<double>(),
					spoilData["min"].get<uint32_t>(),
					spoilData["max"].get<uint32_t>()
				);
		}
	}
	inline void loadPlantSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator(path/"plants"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto& plantSpecies = PlantSpecies::data.emplace_back(
				data["name"].get<std::string>(),
				data["annual"].get<bool>(),
				data["maximumGrowingTemperature"].get<Temperature>(),
				data["minimumGrowingTemperature"].get<Temperature>(),
				data["daysTillDieFromTemperature"].get<uint16_t>() * Config::stepsPerDay,
				data["daysNeedsFluidFrequency"].get<uint16_t>() * Config::stepsPerDay,
				data["volumeFluidConsumed"].get<Volume>(),
				data["daysTillDieWithoutFluid"].get<uint16_t>() * Config::stepsPerDay,
				data["daysTillFullyGrown"].get<uint16_t>() * Config::stepsPerDay,
				data["daysTillFoliageGrowsFromZero"].get<uint16_t>() * Config::stepsPerDay,
				data["growsInSunlight"].get<bool>(),
				data["rootRangeMax"].get<uint32_t>(),
				data["rootRangeMin"].get<uint32_t>(),
				data["adultMass"].get<Mass>(),
				data["dayOfYearForSowStart"].get<uint16_t>(),
				data["dayOfYearForSowEnd"].get<uint16_t>(),
				FluidType::byName(data["fluidType"].get<std::string>())
			);
			if(data.contains("harvestData"))
			{
				auto& harvestData = HarvestData::data.emplace_back(
					data["harvestData"]["dayOfYearToStart"].get<uint16_t>(),
					data["harvestData"]["daysDuration"].get<uint16_t>() * Config::stepsPerDay,
					data["harvestData"]["quantity"].get<uint32_t>(),
					ItemType::byName(data["harvestData"]["itemType"].get<std::string>())
				);
				plantSpecies.harvestData = &harvestData;
			}
			else
				plantSpecies.harvestData = nullptr;
		}
	}
	inline void loadBodyPartTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator(path/"bodyParts"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			BodyPartType& bodyPartType = BodyPartType::data.emplace_back(
				data["name"].get<std::string>(),
				data["volume"].get<Volume>(),
				data["doesLocamotion"].get<bool>(),
				data["doesManipulation"].get<bool>(),
				data.contains("vital")? data["vital"].get<bool>() : false
			);
			for(const Json& pair : data["attackTypesAndMaterials"])
				bodyPartType.attackTypesAndMaterials.emplace_back(loadAttackType(pair.at(0)), &MaterialType::byName(pair.at(1)));
		}
	}
	inline void loadBodyTypes()
	{
		for(const Json& bodyData : tryParse(path/"bodies.json"))
		{
			auto& bodyType = BodyType::data.emplace_back(
				bodyData["name"].get<std::string>()
			);
			for(const Json& bodyPartTypeName : bodyData["bodyPartTypes"])
				bodyType.bodyPartTypes.push_back(&BodyPartType::byName(bodyPartTypeName.get<std::string>()));
		}
	}
	inline void loadAnimalSpecies()
	{
		for(const auto& file : std::filesystem::directory_iterator(path/"animals"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto deathAge = data["deathAgeDays"].get<std::array<uint32_t, 2>>();
			deathAge[0] = deathAge[0] * Config::stepsPerDay;
			deathAge[1] = deathAge[1] * Config::stepsPerDay;
			AnimalSpecies& animalSpecies = AnimalSpecies::data.emplace_back(
				data["name"].get<std::string>(),
				data["sentient"].get<bool>(),
				data["strength"].get<std::array<uint32_t, 3>>(),
				data["dextarity"].get<std::array<uint32_t, 3>>(),
				data["agility"].get<std::array<uint32_t, 3>>(),
				data["mass"].get<std::array<Mass, 3>>(),
				deathAge,
				data["daysTillFullyGrown"].get<uint16_t>() * Config::stepsPerDay,
				data["daysTillDieWithoutFood"].get<uint16_t>() * Config::stepsPerDay,
				data["daysEatFrequency"].get<uint16_t>() * Config::stepsPerDay,
				data["daysTillDieWithoutFluid"].get<uint16_t>() * Config::stepsPerDay,
				data["daysFluidDrinkFrequency"].get<uint16_t>() * Config::stepsPerDay,
				data["daysTillDieInUnsafeTemperature"].get<uint16_t>() * Config::stepsPerDay,
				data["minimumSafeTemperature"].get<Temperature>(),
				data["maximumSafeTemperature"].get<Temperature>(),
				data["daysSleepFrequency"].get<uint16_t>() * Config::stepsPerDay,
				data["hoursTillSleepOveride"].get<uint8_t>() * Config::stepsPerHour,
				data["hoursSleepDuration"].get<uint8_t>() * Config::stepsPerHour,
				data["nocturnal"].get<bool>(),
				data["eatsMeat"].get<bool>(),
				data["eatsLeaves"].get<bool>(),
				data["eatsFruit"].get<bool>(),
				data["visionRange"].get<uint32_t>(),
				data["bodyScale"].get<uint32_t>(),
				MaterialType::byName(data["materialType"].get<std::string>()),
				MoveType::byName(data["moveType"].get<std::string>()),
				FluidType::byName(data["fluidType"].get<std::string>()),
				BodyType::byName(data["bodyType"].get<std::string>())
			);
			assert(data.contains("shapes"));
			for(const auto& shapeName : data["shapes"])
				animalSpecies.shapes.push_back(&Shape::byName(shapeName.get<std::string>()));
		}
	}
	inline void load()
	{
		assert(MaterialType::data.size() == 0);
		assert(std::filesystem::exists(path));
		loadShapes();
		loadFluidTypes();
		loadMaterialTypes();
		loadMoveTypes();
		loadSkillTypes();
		loadItemTypes();
		loadPlantSpecies();
		loadBodyPartTypes();
		loadBodyTypes();
		loadAnimalSpecies();
	}
}
