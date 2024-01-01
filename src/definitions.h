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
#include "craft.h"

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
				data["density"].get<Density>(),
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
				data["density"].get<Density>(),
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
	inline const AttackType loadAttackType(const Json& data, const SkillType& defaultSkill)
	{
		return AttackType(
			data["name"].get<std::string>(),
			data["area"].get<uint32_t>(),
			data["baseForce"].get<Force>(),
			data["range"].get<float>(),
			data["combatScore"].get<uint32_t>(),
			data.contains("coolDownSeconds") ? data["coolDownSeconds"].get<uint32_t>() * Config::stepsPerSecond : 0,
			data.contains("projectile") ? data["projectile"].get<bool>() : false,
			WoundCalculations::byName(data["woundType"].get<std::string>()),
			(data.contains("skillType") ? SkillType::byName(data["skillType"]) : defaultSkill),
			(data.contains("projectileItemType") ? &ItemType::byName(data["projectileItemType"]) : nullptr)
		);
	}
	inline std::pair<ItemQuery, uint32_t> loaditemQuery(const Json& data)
	{
		const ItemType& itemType = ItemType::byName(data["itemType"].get<std::string>());
		uint32_t quantity = data["quantity"].get<uint32_t>();
		ItemQuery query(itemType);
		if(data.contains("materialType"))
			query.m_materialType = &MaterialType::byName(data["materialType"].get<std::string>());
		if(data.contains("materialTypeCategory"))
			query.m_materialTypeCategory = &MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>());
		return std::make_pair(query, quantity);
	}
	inline void loadMaterialTypeConstuctionData()
	{
		for(const Json& data : tryParse(path/"materialConstructionData.json"))
		{
			MaterialConstructionData& constructionData = MaterialConstructionData::data.emplace_back
			(
				 data["name"].get<std::string>(),
				 SkillType::byName(data["skill"]),
				 data["minutesDuration"].get<uint32_t>() * Config::stepsPerMinute
			 );
			for(const Json& item : data["consumed"])
			{
				auto [query, quantity] = loaditemQuery(item);
				constructionData.consumed.emplace_back(query, quantity);
			}
			for(const Json& item : data["unconsumed"])
			{
				auto [query, quantity] = loaditemQuery(item);
				constructionData.unconsumed.emplace_back(query, quantity);
			}
			for(const Json& item : data["byproducts"])
			{
				const ItemType& itemType = ItemType::byName(item["itemType"].get<std::string>());
				uint32_t quantity = item["quantity"].get<uint32_t>();
				const MaterialType* materialType = item.contains("materialType") ?
					&MaterialType::byName(item["materialType"]) :
					nullptr;
				constructionData.byproducts.emplace_back(&itemType, materialType, quantity);
			}
		}

	}
	/*
	inline void loadMedicalProjectTypes()
	{

		for(const auto& file : std::filesystem::directory_iterator(path/"medicalProjectTypes"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto& medicalProjectType = MedicalProjectType::data.emplace_back(
				data["name"].get<std::string>(),
				data["baseMinutesDuration"].get<uint32_t>() * Config::stepsPerMinute
			);
			for(const Json& item : data["consumed"])
			{
				auto [query, quantity] = loaditemQuery(item);
				medicalProjectType.consumedItems.emplace_back(query, quantity);
			}
			for(const Json& item : data["unconsumed"])
			{
				auto [query, quantity] = loaditemQuery(item);
				medicalProjectType.unconsumedItems.emplace_back(query, quantity);
			}
			for(const Json& item : data["byproducts"])
			{
				const ItemType& itemType = ItemType::byName(item["itemType"].get<std::string>());
				uint32_t quantity = item["quantity"].get<uint32_t>();
				const MaterialType* materialType = item.contains("materialType") ?
					&MaterialType::byName(item["materialType"]) :
					nullptr;
				medicalProjectType.byproductItems.emplace_back(&itemType, materialType, quantity);
			}
		}
	}
	*/
	inline void loadCraftJobs()
	{
		for(const Json& data : tryParse(path/"craftStepTypeCategory.json"))
			CraftStepTypeCategory::data.emplace_back(data["name"]);
		for(const auto& file : std::filesystem::directory_iterator(path/"craftJobTypes"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto& craftJobType = CraftJobType::data.emplace_back(
				data["name"].get<std::string>(),
				ItemType::byName(data["productType"].get<std::string>()),
				data["productQuantity"].get<uint32_t>(),
				data.contains("materialTypeCategory") ? &MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>()) : nullptr
			);
			for(const Json& stepData : data["steps"])
			{
				auto& stepType = craftJobType.stepTypes.emplace_back(
					stepData["name"].get<std::string>(),
					CraftStepTypeCategory::byName(stepData["category"].get<std::string>()),
					SkillType::byName(stepData["skillType"].get<std::string>()),
					stepData["minutesDuration"].get<uint32_t>() * Config::stepsPerMinute
				);
				if(stepData.contains("consumed"))
					for(const Json& item : stepData["consumed"])
					{
						auto [query, quantity] = loaditemQuery(item);
						stepType.consumed.emplace_back(query, quantity);
					}
				if(stepData.contains("unconsumed"))
					for(const Json& item : stepData["unconsumed"])
					{
						auto [query, quantity] = loaditemQuery(item);
						stepType.unconsumed.emplace_back(query, quantity);
					}
				if(stepData.contains("byproducts"))
					for(const Json& item : stepData["byproducts"])
					{
						const ItemType& itemType = ItemType::byName(item["itemType"].get<std::string>());
						uint32_t quantity = item["quantity"].get<uint32_t>();
						const MaterialType* materialType = item.contains("materialType") ?
							&MaterialType::byName(item["materialType"]) :
							nullptr;
						stepType.byproducts.emplace_back(&itemType, materialType, quantity);
					}
			}
		}
	}
	inline void loadWeaponsData()
	{
		for(const auto& file : std::filesystem::directory_iterator(path/"items"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto& itemType = ItemType::byNameNonConst(data["name"].get<std::string>());
			if(data.contains("weaponData"))
			{
				Json& weaponData = data["weaponData"];
				const SkillType& skillType = SkillType::byName(weaponData["combatSkill"].get<std::string>());
				auto& weapon = WeaponData::data.emplace_back(
					&skillType,
					weaponData.contains("coolDownSeconds") ? weaponData["coolDownSeconds"].get<float>() * Config::stepsPerSecond : Config::attackCoolDownDurationBaseSteps
				);
				for(Json attackTypeData : weaponData["attackTypes"])
					weapon.attackTypes.push_back(loadAttackType(attackTypeData, skillType));
				itemType.weaponData = &weapon;
			}
			else
				itemType.weaponData = nullptr;
		}
	}
	inline void loadItemTypes()
	{
		for(const auto& file : std::filesystem::directory_iterator(path/"items"))
		{
			if(file.path().extension() != ".json")
				continue;
			Json data = tryParse(file.path());
			auto& itemType = ItemType::data.emplace_back(
				data["name"].get<std::string>(),
				data.contains("installable") && data["installable"].get<bool>() == true,
				Shape::byName(data["shape"].get<std::string>()),
				data["volume"].get<Volume>(),
				data["generic"].get<bool>(),
				data.contains("internalVolume") ? data["internalVolume"].get<Volume>() : 0 ,
				data.contains("canHoldFluids") ? data["canHoldFluids"].get<bool>() : false,
				data["value"].get<uint32_t>(),
				data.contains("edibleForDrinkersOf") ? &FluidType::byName(data["edibleForDrinkersOf"].get<std::string>()) : nullptr,
				MoveType::byName(data.contains("moveType") ? data["moveType"].get<std::string>() : "none")
			);
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
		// Now that item types are loaded we can load material type spoil and construction data.
		loadMaterialTypeConstuctionData();
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
			// Construction data
			if(data.contains("constructionData"))
				materialType.constructionData = &MaterialConstructionData::byNameSpecialized(data["constructionData"].get<std::string>(), materialType);
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
				FluidType::byName(data["fluidType"].get<std::string>()),
				data.contains("maxWildGrowth") ? data["maxWildGrowth"].get<uint8_t>() : 0,
				data.contains("isTree") ? data["isTree"].get<bool>() : false,
				data.contains("logsGeneratedByFellingWhenFullGrown") ? data["logsGeneratedByFellingWhenFullGrown"].get<uint32_t>() : 0,
				data.contains("branchesGeneratedByFellingWhenFullGrown") ? data["branchesGeneratedByFellingWhenFullGrown"].get<uint32_t>() : 0,
				data.contains("woodType") ? data["woodType"].get<const MaterialType*>() : nullptr
			);
			assert(data.contains("shapes"));
			for(const auto& shapeName : data["shapes"])
				plantSpecies.shapes.push_back(&Shape::byName(shapeName.get<std::string>()));
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
			const SkillType& unarmedSkill = SkillType::byName("unarmed");
			for(const Json& pair : data["attackTypesAndMaterials"])
				bodyPartType.attackTypesAndMaterials.emplace_back(loadAttackType(pair.at(0), unarmedSkill), &MaterialType::byName(pair.at(1)));
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
		loadWeaponsData();
		loadMaterialTypeConstuctionData();
		loadCraftJobs();
		//loadMedicalProjectTypes();
		loadPlantSpecies();
		loadBodyPartTypes();
		loadBodyTypes();
		loadAnimalSpecies();
	}
}
