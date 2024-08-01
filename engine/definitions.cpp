#include "definitions.h"
#include "config.h"
#include "materialType.h"
#include "fluidType.h"
#include "body.h"
#include "animalSpecies.h"
#include "moveType.h"
#include "attackType.h"
#include "skill.h"
#include "craft.h"
#include "types.h"
#include "itemType.h"

#include <fstream>
#include <algorithm>

using Json = nlohmann::json;
Json definitions::tryParse(std::string filePath)
{
	std::ifstream f(filePath);
	return Json::parse(f);
}
void definitions::loadShapes()
{
	Json data = tryParse(path/"shapes.json");
	for(const Json& shapeData : data)
	{
		shapeDataStore.emplace_back(
			shapeData["name"].get<std::string>(),
			shapeData["positions"].get<std::vector<std::array<int32_t, 4>>>(),
			// TODO: move this to ui.
			shapeData.contains("displayScale") ? shapeData["displayScale"].get<uint32_t>() : 1
		);
	}
}
void definitions::loadFluidTypes()
{
	assert(std::filesystem::exists(path/"fluids"));
	// Don't do freezesInto here because we haven't loaded soid materials yet.
	for(const auto& file : std::filesystem::directory_iterator(path/"fluids"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		fluidTypeDataStore.emplace_back(
			data["name"].get<std::string>(),
			data["viscosity"].get<int32_t>(),
			data["density"].get<Density>(),
			//TODO: store as seconds rather then steps
			data.contains("mistDuration") ? data["mistDuration"].get<Step>() : Step::create(0),
			data.contains("mistDuration") ? data["maxMistSpread"].get<DistanceInBlocks>() : DistanceInBlocks::create(0)
		);
	}
}
void definitions::loadMoveTypes()
{
	Json data = tryParse(path/"moveTypes.json");
	for(const Json& moveTypeData : data)
	{
		moveTypeDataStore.emplace_back(
			moveTypeData["name"].get<std::string>(),
			moveTypeData["walk"].get<bool>(),
			moveTypeData["climb"].get<uint32_t>(),
			moveTypeData["jumpDown"].get<bool>(),
			moveTypeData["fly"].get<bool>(),
			moveTypeData.contains("breathless"),
			moveTypeData.contains("onlyBreathsFluids")
		);
		for(auto& pair : moveTypeData["swim"].items())
			moveTypeDataStore.back().swim[&FluidType::byName(pair.key())] = pair.value();
		if(moveTypeData.contains("breathableFluids"))
			for(const Json& name : moveTypeData["breathableFluids"])
				moveTypeDataStore.back().breathableFluids.insert(&FluidType::byName(name));
	}
}
void definitions::loadMaterialTypes()
{
	for(const Json& data : tryParse(path/"materialTypeCategories.json"))
		materialTypeCategoryDataStore.emplace_back(data["name"]);
	for(const auto& file : std::filesystem::directory_iterator(path/"materials"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		auto& materialType = materialTypeDataStore.emplace_back(
			data["name"].get<std::string>(),
			data["density"].get<Density>(),
			data["hardness"].get<uint32_t>(),
			data["transparent"].get<bool>()
		);
		if(data.contains("category"))
			materialType.materialTypeCategory = &MaterialTypeCategory::byName(data["category"].get<std::string>());
		if(data.contains("meltingPoint"))
		{
		       	materialType.meltingPoint = data["meltingPoint"].get<Temperature>();
			assert(data.contains("meltsInto"));
			materialType.meltsInto = &FluidType::byName(data["meltsInto"].get<std::string>());
		}
		if(data.contains("burnData"))
			materialType.burnData = &burnDataStore.emplace_back(
					// TODO: store as seconds rather then steps.
					data["burnData"]["burnStageDuration"].get<Step>(),
					data["burnData"]["flameStageDuration"].get<Step>(),
					data["burnData"]["ignitionTemperature"].get<Temperature>(),
					data["burnData"]["flameTemperature"].get<Temperature>()
			);
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
void definitions::loadSkillTypes()
{
	for(const Json& skillData : tryParse(path/"skills.json"))
	{
		skillTypeDataStore.emplace_back(
			skillData["name"].get<std::string>(),
			skillData["xpPerLevelModifier"].get<float>(),
			skillData["level1Xp"].get<uint32_t>()
		);
	}
}
const AttackType definitions::loadAttackType(const Json& data, const SkillType& defaultSkill)
{
	return AttackType(
		data["name"].get<std::string>(),
		data["area"].get<uint32_t>(),
		data["baseForce"].get<Force>(),
		data["range"].get<DistanceInBlocksFractional>(),
		data["combatScore"].get<uint32_t>(),
		data.contains("coolDownSeconds") ? Config::stepsPerSecond * data["coolDownSeconds"].get<uint32_t>() : Step::create(0),
		data.contains("projectile") ? data["projectile"].get<bool>() : false,
		WoundCalculations::byName(data["woundType"].get<std::string>()),
		(data.contains("skillType") ? SkillType::byName(data["skillType"]) : defaultSkill),
		(data.contains("projectileItemType") ? &ItemType::byName(data["projectileItemType"]) : nullptr)
	);
}
std::pair<ItemQuery, Quantity> definitions::loaditemQuery(const Json& data)
{
	const ItemType& itemType = ItemType::byName(data["itemType"].get<std::string>());
	Quantity quantity = data["quantity"].get<Quantity>();
	ItemQuery query(itemType);
	if(data.contains("materialType"))
		query.m_materialType = &MaterialType::byName(data["materialType"].get<std::string>());
	if(data.contains("materialTypeCategory"))
		query.m_materialTypeCategory = &MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>());
	return std::make_pair(query, quantity);
}
void definitions::loadMaterialTypeConstuctionData()
{
	for(const Json& data : tryParse(path/"materialConstructionData.json"))
	{
		MaterialConstructionData& constructionData = materialConstructionDataStore.emplace_back
		(
			 data["name"].get<std::string>(),
			 SkillType::byName(data["skill"]),
			 Config::stepsPerMinute * data["minutesDuration"].get<uint32_t>()
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
			Quantity quantity = item["quantity"].get<Quantity>();
			const MaterialType* materialType = item.contains("materialType") ?
				&MaterialType::byName(item["materialType"]) :
				nullptr;
			constructionData.byproducts.emplace_back(&itemType, materialType, quantity);
		}
	}

}
/*
void definitions::loadMedicalProjectTypes()
{

	for(const auto& file : std::filesystem::directory_iterator(path/"medicalProjectTypes"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		auto& medicalProjectType = medicalProjectTypeDataStore.emplace_back(
			data["name"].get<std::string>(),
			Config::stepsPerMinute * data["baseMinutesDuration"].get<uint32_t>()
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
void definitions::loadCraftJobs()
{
	for(const Json& data : tryParse(path/"craftStepTypeCategory.json"))
		craftStepTypeCategoryDataStore.emplace_back(data["name"]);
	for(const auto& file : std::filesystem::directory_iterator(path/"craftJobTypes"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		auto& craftJobType = craftJobTypeDataStore.emplace_back(
			data["name"].get<std::string>(),
			ItemType::byName(data["productType"].get<std::string>()),
			data["productQuantity"].get<Quantity>(),
			data.contains("materialTypeCategory") ? &MaterialTypeCategory::byName(data["materialTypeCategory"].get<std::string>()) : nullptr
		);
		for(const Json& stepData : data["steps"])
		{
			auto& stepType = craftJobType.stepTypes.emplace_back(
				stepData["name"].get<std::string>(),
				CraftStepTypeCategory::byName(stepData["category"].get<std::string>()),
				SkillType::byName(stepData["skillType"].get<std::string>()),
				Config::stepsPerMinute * stepData["minutesDuration"].get<uint32_t>()
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
					Quantity quantity = item["quantity"].get<Quantity>();
					const MaterialType* materialType = item.contains("materialType") ?
						&MaterialType::byName(item["materialType"]) :
						nullptr;
					stepType.byproducts.emplace_back(&itemType, materialType, quantity);
				}
		}
	}
}
void definitions::loadWeaponsData()
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
			auto& weapon = weaponDataStore.emplace_back(
				&skillType,
				weaponData.contains("coolDownSeconds") ? Config::stepsPerSecond * weaponData["coolDownSeconds"].get<float>() : Config::attackCoolDownDurationBaseSteps
			);
			for(Json attackTypeData : weaponData["attackTypes"])
				weapon.attackTypes.push_back(loadAttackType(attackTypeData, skillType));
			itemType.weaponData = &weapon;
		}
		else
			itemType.weaponData = nullptr;
	}
}
void definitions::loadItemTypes()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"items"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		auto& itemType = itemTypeDataStore.emplace_back(
			data["name"].get<std::string>(),
			Shape::byName(data["shape"].get<std::string>()),
			MoveType::byName(data.contains("moveType") ? data["moveType"].get<std::string>() : "none"),
			data["volume"].get<Volume>(),
			data.contains("internalVolume") ? data["internalVolume"].get<Volume>() : Volume::create(0) ,
			data["value"].get<uint32_t>(),
			data.contains("installable"),
			data.contains("generic") && data["generic"].get<bool>(),
			data.contains("canHoldFluids")
		);
		if(data.contains("edibleForDrinkersOf"))
			itemType.edibleForDrinkersOf = &FluidType::byName(data["edibleForDrinkersOf"].get<std::string>());
		if(data.contains("wearableData"))
		{
			Json& wearable = data["wearableData"];
			auto& wearableData = wearableDataStore.emplace_back(
				wearable["defenseScore"].get<uint32_t>(),
				wearable["layer"].get<uint32_t>(),
				Config::scaleOfHumanBody,
				wearable["forceAbsorbedUnpiercedModifier"].get<uint32_t>(),
				wearable["forceAbsorbedPiercedModifier"].get<uint32_t>(),
				wearable["percentCoverage"].get<Percent>(),
				wearable["rigid"].get<bool>()
			);
			for(const auto& bodyPartName : data["bodyPartsCovered"])
				wearableData.bodyPartsCovered.push_back(&BodyPartType::byName(bodyPartName.get<std::string>()));
			itemType.wearableData = &wearableData;
		}
		else
			itemType.wearableData = nullptr;
		if(data.contains("materialTypeCategories"))
			for(const Json& materialTypeCategoryName : data["materialTypeCategories"])
				itemType.materialTypeCategories.push_back(&MaterialTypeCategory::byName(materialTypeCategoryName.get<std::string>()));
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
				spoilData["min"].get<Quantity>(),
				spoilData["max"].get<Quantity>()
			);
		// Construction data
		if(data.contains("constructionData"))
			materialType.constructionData = &MaterialConstructionData::byNameSpecialized(data["constructionData"].get<std::string>(), materialType);
	}
}
void definitions::loadPlantSpecies()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"plants"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		auto& plantSpecies = plantSpeciesDataStore.emplace_back(
			data["name"].get<std::string>(),
			data["annual"].get<bool>(),
			data["maximumGrowingTemperature"].get<Temperature>(),
			data["minimumGrowingTemperature"].get<Temperature>(),
			Config::stepsPerDay * data["daysTillDieFromTemperature"].get<uint16_t>(),
			Config::stepsPerDay * data["daysNeedsFluidFrequency"].get<uint16_t>(),
			data["volumeFluidConsumed"].get<Volume>(),
			Config::stepsPerDay * data["daysTillDieWithoutFluid"].get<uint16_t>(),
			Config::stepsPerDay * data["daysTillFullyGrown"].get<uint16_t>(),
			Config::stepsPerDay * data["daysTillFoliageGrowsFromZero"].get<uint16_t>(),
			data["growsInSunlight"].get<bool>(),
			data["rootRangeMax"].get<DistanceInBlocks>(),
			data["rootRangeMin"].get<DistanceInBlocks>(),
			data["adultMass"].get<Mass>(),
			data["dayOfYearForSowStart"].get<uint16_t>(),
			data["dayOfYearForSowEnd"].get<uint16_t>(),
			data.contains("isTree") ? data["isTree"].get<bool>() : false,
			data.contains("logsGeneratedByFellingWhenFullGrown") ? data["logsGeneratedByFellingWhenFullGrown"].get<Quantity>() : Quantity::create(0),
			data.contains("branchesGeneratedByFellingWhenFullGrown") ? data["branchesGeneratedByFellingWhenFullGrown"].get<Quantity>() : Quantity::create(0),
			data.contains("maxWildGrowth") ? data["maxWildGrowth"].get<uint8_t>() : 0,
			FluidType::byName(data["fluidType"].get<std::string>()),
			data.contains("woodType") ? data["woodType"].get<const MaterialType*>() : nullptr
		);
		assert(data.contains("shapes"));
		for(const auto& shapeName : data["shapes"])
			plantSpecies.shapes.push_back(&Shape::byName(shapeName.get<std::string>()));
		if(data.contains("harvestData"))
		{
			auto& harvestData = harvestDataStore.emplace_back(
				data["harvestData"]["dayOfYearToStart"].get<uint16_t>(),
				Config::stepsPerDay * data["harvestData"]["daysDuration"].get<uint16_t>(),
				data["harvestData"]["quantity"].get<Quantity>(),
				ItemType::byName(data["harvestData"]["itemType"].get<std::string>())
			);
			plantSpecies.harvestData = &harvestData;
		}
		else
			plantSpecies.harvestData = nullptr;
	}
}
void definitions::loadBodyPartTypes()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"bodyParts"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		BodyPartType& bodyPartType = bodyPartTypeDataStore.emplace_back(
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
void definitions::loadBodyTypes()
{
	for(const Json& bodyData : tryParse(path/"bodies.json"))
	{
		auto& bodyType = bodyTypeDataStore.emplace_back(
			bodyData["name"].get<std::string>()
		);
		for(const Json& bodyPartTypeName : bodyData["bodyPartTypes"])
			bodyType.bodyPartTypes.push_back(&BodyPartType::byName(bodyPartTypeName.get<std::string>()));
	}
}
void definitions::loadAnimalSpecies()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"animals"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		auto deathAgeDays = data["deathAgeDays"].get<std::array<uint32_t, 2>>();
		std::array<Step, 2> deathAge;
		deathAge[0] = Config::stepsPerDay * deathAgeDays[0];
		deathAge[1] = Config::stepsPerDay * deathAgeDays[1];
		AnimalSpecies& animalSpecies = animalSpeciesDataStore.emplace_back(
			data["name"].get<std::string>(),
			data["sentient"].get<bool>(),
			data["strength"].get<std::array<uint32_t, 3>>(),
			data["dextarity"].get<std::array<uint32_t, 3>>(),
			data["agility"].get<std::array<uint32_t, 3>>(),
			data["mass"].get<std::array<Mass, 3>>(),
			deathAge,
			Config::stepsPerDay * data["daysTillFullyGrown"].get<uint16_t>(),
			Config::stepsPerDay * data["daysTillDieWithoutFood"].get<uint16_t>(),
			Config::stepsPerDay * data["daysEatFrequency"].get<uint16_t>(),
			Config::stepsPerDay * data["daysTillDieWithoutFluid"].get<uint16_t>(),
			Config::stepsPerDay * data["daysFluidDrinkFrequency"].get<uint16_t>(),
			Config::stepsPerDay * data["daysTillDieInUnsafeTemperature"].get<uint16_t>(),
			data["minimumSafeTemperature"].get<Temperature>(),
			data["maximumSafeTemperature"].get<Temperature>(),
			Config::stepsPerDay * data["daysSleepFrequency"].get<uint16_t>(),
			Config::stepsPerHour * data["hoursTillSleepOveride"].get<uint8_t>(),
			Config::stepsPerHour * data["hoursSleepDuration"].get<uint8_t>(),
			data["nocturnal"].get<bool>(),
			data["eatsMeat"].get<bool>(),
			data["eatsLeaves"].get<bool>(),
			data["eatsFruit"].get<bool>(),
			data["visionRange"].get<DistanceInBlocks>(),
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
void definitions::load()
{
	assert(materialTypeDataStore.size() == 0);
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
