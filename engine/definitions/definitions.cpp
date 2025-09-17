#include "definitions/definitions.h"
#include "config.h"
#include "definitions/materialType.h"
#include "fluidType.h"
#include "body.h"
#include "definitions/bodyType.h"
#include "definitions/animalSpecies.h"
#include "definitions/moveType.h"
#include "definitions/attackType.h"
#include "actors/skill.h"
#include "craft.h"
#include "numericTypes/types.h"
#include "definitions/itemType.h"
#include "definitions/shape.h"
#include "definitions/plantSpecies.h"

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
	Json shapeData = tryParse(path/"shapes.json");
	for(const Json& data : shapeData)
	{
		Shape::create(
			(data["name"].get<std::string>()),
			data["positions"].get<MapWithOffsetCuboidKeys<CollisionVolume>>(),
			// TODO: move this to ui.
			data.contains("displayScale") ? data["displayScale"].get<uint32_t>() : 1
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
		FluidTypeParamaters params{
			.name=data["name"].get<std::string>(),
			.viscosity=data["viscosity"].get<uint32_t>(),
			.density=data["density"].get<Density>(),
			//TODO: Store as seconds rather then steps.
			//TODO: Initalize to null rather then 0.
			.mistDuration=data.contains("mistDuration") ? data["mistDuration"].get<Step>() : Step::create(0),
			.maxMistSpread=data.contains("mistDuration") ? data["maxMistSpread"].get<Distance>() : Distance::create(0),
			.evaporationRate=data.contains("evaporationRate") ? data["evaporationRate"].get<float>() : 0.f,
		};
		FluidType::create(params);
	}
}
void definitions::loadMoveTypes()
{
	Json moveTypeData = tryParse(path/"moveTypes.json");
	for(const Json& data : moveTypeData)
	{
		MoveTypeParamaters p{
			.name=(data["name"].get<std::string>()),
			.surface=data["surface"].get<bool>(),
			.stairs=data["stairs"].get<bool>(),
			.climb=data["climb"].get<uint8_t>(),
			.jumpDown=data["jumpDown"].get<bool>(),
			.fly=data["fly"].get<bool>(),
			.breathless=data.contains("breathless"),
			.onlyBreathsFluids=data.contains("onlyBreathsFluids"),
			.floating=(data.contains("floating") && data["floating"])
		};
		for(auto& pair : data["swim"].items())
			p.swim.insert(FluidType::byName(pair.key()), pair.value().get<CollisionVolume>());
		if(data.contains("breathableFluids"))
			for(const Json& name : data["breathableFluids"])
				p.breathableFluids.insert(FluidType::byName(name));
		MoveType::create(p);
	}
}
void definitions::loadMaterialTypes()
{
	for(const Json& data : tryParse(path/"materialTypeCategories.json"))
		MaterialTypeCategory::create((data["name"].get<std::string>()));
	for(const auto& file : std::filesystem::directory_iterator(path/"materials"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		MaterialTypeParamaters p{
			.name=(data["name"].get<std::string>()),
			.density=data["density"].get<Density>(),
			.hardness=data["hardness"].get<uint32_t>(),
			.transparent=data["transparent"].get<bool>(),
			.valuePerUnitFullDisplacement=data["value"].get<float>(),
		};
		if(data.contains("category"))
			p.materialTypeCategory = MaterialTypeCategory::byName((data["category"].get<std::string>()));
		if(data.contains("meltingPoint"))
		{
		    	p.meltingPoint = data["meltingPoint"].get<Temperature>();
			assert(data.contains("meltsInto"));
			p.meltsInto = FluidType::byName(data["meltsInto"].get<std::string>());
		}
		if(data.contains("burnData"))
		{
			// TODO: store as seconds rather then steps.
			data["burnData"]["burnStageDuration"].get_to(p.burnStageDuration);
			data["burnData"]["flameStageDuration"].get_to(p.flameStageDuration);
			data["burnData"]["ignitionTemperature"].get_to(p.ignitionTemperature);
			data["burnData"]["flameTemperature"].get_to(p.flameTemperature);
		}
		MaterialType::create(p);
	}
	// Load FluidType freeze into here, now that solid material types are loaded.
	for(const auto& file : std::filesystem::directory_iterator(path/"fluids"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		if(data.contains("freezesInto"))
		{
			FluidTypeId fluidType = FluidType::byName(data["name"].get<std::string>());
			MaterialTypeId materialType = MaterialType::byName((data["freezesInto"].get<std::string>()));
			FluidType::setFreezesInto(fluidType, materialType);
		}
	}
}
void definitions::loadSkillTypes()
{
	for(const Json& data : tryParse(path/"skills.json"))
	{
		SkillTypeParamaters p{
			.name=(data["name"].get<std::string>()),
			.xpPerLevelModifier=data["xpPerLevelModifier"].get<float>(),
			.level1Xp=data["level1Xp"].get<SkillExperiencePoints>()
		};
		SkillType::create(p);
	}
}
AttackTypeId definitions::loadAttackType(const Json& data, const SkillTypeId& defaultSkill)
{
	AttackTypeParamaters p{
		.name=(data["name"].get<std::string>()),
		.area=data["area"].get<uint32_t>(),
		.baseForce=data["baseForce"].get<Force>(),
		.range=data["range"].get<DistanceFractional>(),
		.combatScore=data["combatScore"].get<CombatScore>(),
		.coolDown=data.contains("coolDownSeconds") ? Config::stepsPerSecond * data["coolDownSeconds"].get<uint32_t>() * Config::stepsPerSecond : Step::create(0),
		.projectile=data.contains("projectile") ? data["projectile"].get<bool>() : false,
		.woundType=WoundCalculations::byName((data["woundType"].get<std::string>())),
		.skillType=data.contains("skillType") ? SkillType::byName((data["skillType"].get<std::string>())) : defaultSkill,
		.projectileItemType=data.contains("projectileItemType") ? ItemType::byName((data["projectileItemType"].get<std::string>())) : ItemTypeId::null()
	};
	return AttackType::create(p);
}
std::pair<ItemQuery, Quantity> definitions::loadItemQuery(const Json& data)
{
	ItemTypeId itemType = ItemType::byName((data["itemType"].get<std::string>()));
	Quantity quantity = data["quantity"].get<Quantity>();
	ItemQuery query = ItemQuery::create(itemType);
	if(data.contains("materialType"))
		query.m_solid = MaterialType::byName((data["materialType"].get<std::string>()));
	if(data.contains("materialTypeCategory"))
		query.m_solidCategory = MaterialTypeCategory::byName((data["materialTypeCategory"].get<std::string>()));
	return std::make_pair(query, quantity);
}
std::unordered_map<std::string, MaterialTypeConstructionDataParamaters> definitions::loadMaterialTypeConstuctionData()
{
	std::unordered_map<std::string, MaterialTypeConstructionDataParamaters> output;
	for(const Json& data : tryParse(path/"materialConstructionData.json"))
	{
		MaterialTypeConstructionDataParamaters p{
			.name=(data["name"].get<std::string>()),
			.skill=SkillType::byName((data["skill"].get<std::string>())),
			.duration=Config::stepsPerMinute * data["minutesDuration"].get<uint32_t>()
		};
		for(const Json& item : data["consumed"])
		{
			auto [query, quantity] = loadItemQuery(item);
			p.consumed.emplace_back(query, quantity);
		}
		for(const Json& item : data["unconsumed"])
		{
			auto [query, quantity] = loadItemQuery(item);
			p.unconsumed.emplace_back(query, quantity);
		}
		for(const Json& item : data["byproducts"])
		{
			ItemTypeId itemType = ItemType::byName((item["itemType"].get<std::string>()));
			Quantity quantity = item["quantity"].get<Quantity>();
			MaterialTypeId materialType = item.contains("materialType") ?
				MaterialType::byName((item["materialType"].get<std::string>())) :
				MaterialTypeId::null();
			p.byproducts.emplace_back(itemType, materialType, quantity);
		}
		output[p.name] = p;
	}
	return output;
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
			(data["name"].get<std::string>()),
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
			ItemTypeId itemType = ItemType::byName(item["itemType"].get<std::string>());
			uint32_t quantity = item["quantity"].get<uint32_t>();
			MaterialTypeId materialType = item.contains("materialType") ?
				&MaterialType::byName((item["materialType"].get<std::string>())) :
				nullptr;
			medicalProjectType.byproductItems.emplace_back(&itemType, materialType, quantity);
		}
	}
}
*/
void definitions::loadCraftJobs()
{
	for(const Json& data : tryParse(path/"craftStepTypeCategory.json"))
		CraftStepTypeCategory::create((data["name"].get<std::string>()));
	for(const auto& file : std::filesystem::directory_iterator(path/"craftJobTypes"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		std::vector<CraftStepType> stepTypes;
		for(const Json& stepData : data["steps"])
		{
			CraftStepType& craftStepType = stepTypes.emplace_back(
				(stepData["name"].get<std::string>()),
				CraftStepTypeCategory::byName((stepData["category"].get<std::string>())),
				SkillType::byName((stepData["skillType"].get<std::string>())),
				Config::stepsPerMinute * stepData["minutesDuration"].get<uint32_t>()
			);
			if(stepData.contains("consumed"))
				for(const Json& item : stepData["consumed"])
				{
					auto [query, quantity] = loadItemQuery(item);
					craftStepType.consumed.emplace_back(query, quantity);
				}
			if(stepData.contains("unconsumed"))
				for(const Json& item : stepData["unconsumed"])
				{
					auto [query, quantity] = loadItemQuery(item);
					craftStepType.unconsumed.emplace_back(query, quantity);
				}
			if(stepData.contains("byproducts"))
				for(const Json& item : stepData["byproducts"])
				{
					ItemTypeId itemType = ItemType::byName((item["itemType"].get<std::string>()));
					Quantity quantity = item["quantity"].get<Quantity>();
					MaterialTypeId materialType = item.contains("materialType") ?
						MaterialType::byName((item["materialType"].get<std::string>())) :
						MaterialTypeId::null();
					craftStepType.byproducts.emplace_back(itemType, materialType, quantity);
				}
		}

		CraftJobType::create(
			(data["name"].get<std::string>()),
			ItemType::byName((data["productType"].get<std::string>())),
			data["productQuantity"].get<Quantity>(),
			data.contains("materialTypeCategory") ? MaterialTypeCategory::byName((data["materialTypeCategory"].get<std::string>())) : MaterialCategoryTypeId::null(),
			stepTypes
		);
	}
}
void definitions::loadItemTypes()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"items"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		ItemTypeParamaters p{
			.name=(data["name"].get<std::string>()),
			.shape=Shape::byName((data["shape"].get<std::string>())),
			.moveType=MoveType::byName(data.contains("moveType") ? (data["moveType"].get<std::string>()) : "none"),
			.volume=data["volume"].get<FullDisplacement>(),
			.internalVolume=data.contains("internalVolume") ? data["internalVolume"].get<FullDisplacement>() : FullDisplacement::create(0) ,
			.value=data["value"].get<uint32_t>(),
			.installable=data.contains("installable"),
			.generic=data.contains("generic") && data["generic"].get<bool>(),
			.canHoldFluids=data.contains("canHoldFluids")
		};
		if(data.contains("motiveForce"))
			data["motiveForce"].get_to(p.motiveForce);
		if(data.contains("decks"))
			data["decks"].get_to(p.decks);
		if(data.contains("edibleForDrinkersOf"))
			p.edibleForDrinkersOf = FluidType::byName(data["edibleForDrinkersOf"].get<std::string>());
		if(data.contains("wearableData"))
		{
			Json& wearable = data["wearableData"];
			wearable["layer"].get_to(p.wearable_layer);
			wearable["defenseScore"].get_to(p.wearable_defenseScore);
			//TODO
			p.wearable_bodyTypeScale = Config::scaleOfHumanBody;
			wearable["forceAbsorbedUnpiercedModifier"].get_to(p.wearable_forceAbsorbedUnpiercedModifier);
			wearable["forceAbsorbedPiercedModifier"].get_to(p.wearable_forceAbsorbedPiercedModifier);
			wearable["percentCoverage"].get_to(p.wearable_percentCoverage);
			wearable["rigid"].get_to(p.wearable_rigid);
			for(const auto& bodyPartName : data["bodyPartsCovered"])
				p.wearable_bodyPartsCovered.push_back(BodyPartType::byName((bodyPartName.get<std::string>())));
		}
		if(data.contains("materialTypeCategories"))
			for(const Json& materialTypeCategoryName : data["materialTypeCategories"])
				p.materialTypeCategories.push_back(MaterialTypeCategory::byName((materialTypeCategoryName.get<std::string>())));
		if(data.contains("weaponData"))
		{
			SkillTypeId skill = SkillType::byName((data["weaponData"]["combatSkill"]));
			for (const Json& attackTypeData : data["weaponData"]["attackTypes"])
				p.attackTypes.emplace_back(loadAttackType(attackTypeData, skill));
			p.attackCoolDownBase = data["weaponData"].contains("coolDownSeconds") ?
				Step::create((float)Config::stepsPerSecond.get() * data["weaponData"]["coolDownSeconds"].get<float>()) :
				Config::attackCoolDownDurationBaseSteps;
		}
		ItemType::create(p);
	}
	// Now that item types are loaded we can load material type spoil and construction data.
	auto constructionData = loadMaterialTypeConstuctionData();
	for(const auto& file : std::filesystem::directory_iterator(path/"materials"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		MaterialTypeId materialType = MaterialType::byName((data["name"].get<std::string>()));
		for(Json& spoilData : data["spoilData"])
		{
			MaterialTypeId spoilMaterialType = MaterialType::byName((spoilData["materialType"].get<std::string>()));
			auto& spoils = MaterialType::getSpoilData(materialType);
			spoils.push_back(SpoilData::create(
				spoilMaterialType,
				ItemType::byName((spoilData["itemType"].get<std::string>())),
				spoilData["chance"].get<Percent>(),
				spoilData["min"].get<Quantity>(),
				spoilData["max"].get<Quantity>()
			));
		}
		if(data.contains("constructionData"))
		{
			MaterialTypeConstructionDataParamaters& constructionP = constructionData.at((data["constructionData"].get<std::string>()));
			// Make a copy of Query so it can be specialized.
			auto consumed = constructionP.consumed;
			for(auto& [itemQuery, quantity] : consumed)
				if(itemQuery.m_solid.empty())
					itemQuery.m_solid = materialType;
			auto byproducts = constructionP.byproducts;
			for(auto& tuple : byproducts)
				if(std::get<1>(tuple).empty())
					std::get<1>(tuple) = materialType;
			MaterialType::setConstructionParamaters(materialType, constructionP);
		}
	}
}
void definitions::loadPlantSpecies()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"plants"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		PlantSpeciesParamaters plantSpeciesParamaters = PlantSpeciesParamaters{
			.name=(data["name"].get<std::string>()),
			.fluidType=FluidType::byName(data["fluidType"].get<std::string>()),
			.woodType=data.contains("woodType") ? MaterialType::byName((data["woodType"].get<std::string>())) : MaterialTypeId::null(),
			.stepsNeedsFluidFrequency=Config::stepsPerDay * data["daysNeedsFluidFrequency"].get<uint16_t>(),
			.stepsTillDieWithoutFluid=Config::stepsPerDay * data["daysTillDieWithoutFluid"].get<uint16_t>(),
			.stepsTillFullyGrown=Config::stepsPerDay * data["daysTillFullyGrown"].get<uint16_t>(),
			.stepsTillFoliageGrowsFromZero=Config::stepsPerDay * data["daysTillFoliageGrowsFromZero"].get<uint16_t>(),
			.stepsTillDieFromTemperature=Config::stepsPerDay * data["daysTillDieFromTemperature"].get<uint16_t>(),
			.rootRangeMax=data["rootRangeMax"].get<Distance>(),
			.rootRangeMin=data["rootRangeMin"].get<Distance>(),
			.logsGeneratedByFellingWhenFullGrown=data.contains("logsGeneratedByFellingWhenFullGrown") ? data["logsGeneratedByFellingWhenFullGrown"].get<Quantity>() : Quantity::create(0),
			.branchesGeneratedByFellingWhenFullGrown=data.contains("branchesGeneratedByFellingWhenFullGrown") ? data["branchesGeneratedByFellingWhenFullGrown"].get<Quantity>() : Quantity::create(0),
			.adultMass=data["adultMass"].get<Mass>(),
			.maximumGrowingTemperature=data["maximumGrowingTemperature"].get<Temperature>(),
			.minimumGrowingTemperature=data["minimumGrowingTemperature"].get<Temperature>(),
			.volumeFluidConsumed=data["volumeFluidConsumed"].get<FullDisplacement>(),
			.dayOfYearForSowStart=data["dayOfYearForSowStart"].get<uint16_t>(),
			.dayOfYearForSowEnd=data["dayOfYearForSowEnd"].get<uint16_t>(),
			.maxWildGrowth=data.contains("maxWildGrowth") ? data["maxWildGrowth"].get<uint8_t>() : (uint8_t)0,
			.annual=data["annual"].get<bool>(),
			.growsInSunLight=data["growsInSunlight"].get<bool>(),
			.isTree=data.contains("isTree") ? data["isTree"].get<bool>() : false,
		};
		if(data.contains("harvestData"))
		{
			plantSpeciesParamaters.fruitItemType = ItemType::byName((data["harvestData"]["itemType"].get<std::string>()));
			plantSpeciesParamaters.stepsDurationHarvest = Config::stepsPerDay * data["harvestData"]["daysDuration"].get<uint16_t>();
			plantSpeciesParamaters.itemQuantityToHarvest = data["harvestData"]["quantity"].get<Quantity>();
			plantSpeciesParamaters.dayOfYearToStartHarvest = data["harvestData"]["dayOfYearToStart"].get<uint16_t>();
		}
		assert(data.contains("shapes"));
		for(const auto& shapeName : data["shapes"])
			plantSpeciesParamaters.shapes.push_back(Shape::byName((shapeName.get<std::string>())));
		PlantSpecies::create(plantSpeciesParamaters);
	}
}
void definitions::loadBodyPartTypes()
{
	for(const auto& file : std::filesystem::directory_iterator(path/"bodyParts"))
	{
		if(file.path().extension() != ".json")
			continue;
		Json data = tryParse(file.path());
		BodyPartTypeParamaters p{
			.name=(data["name"].get<std::string>()),
			.volume=data["volume"].get<FullDisplacement>(),
			.doesLocamotion=data["doesLocamotion"].get<bool>(),
			.doesManipulation=data["doesManipulation"].get<bool>(),
			.vital=data.contains("vital")? data["vital"].get<bool>() : false,
			.attackTypesAndMaterials = {},

		};
		SkillTypeId unarmedSkill = SkillType::byName("unarmed");
		for(const Json& pair : data["attackTypesAndMaterials"])
			p.attackTypesAndMaterials.emplace_back(loadAttackType(pair.at(0), unarmedSkill), MaterialType::byName((pair.at(1).get<std::string>())));
		BodyPartType::create(p);
	}
}
void definitions::loadBodyTypes()
{
	for(const Json& bodyData : tryParse(path/"bodies.json"))
	{
		std::vector<BodyPartTypeId> parts;
		for(const Json& bodyPartTypeName : bodyData["bodyPartTypes"])
			parts.push_back(BodyPartType::byName((bodyPartTypeName.get<std::string>())));
		BodyType::create(
			(bodyData["name"].get<std::string>()),
			parts
		);
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
		AnimalSpeciesParamaters params{
			.name=(data["name"].get<std::string>()),
			.sentient=data["sentient"].get<bool>(),
			.strength=data["strength"].get<std::array<AttributeLevel, 3>>(),
			.dextarity=data["dextarity"].get<std::array<AttributeLevel, 3>>(),
			.agility=data["agility"].get<std::array<AttributeLevel, 3>>(),
			.mass=data["mass"].get<std::array<Mass, 3>>(),
			.height=data["height"].get<std::array<uint32_t, 3>>(),
			.deathAgeSteps=deathAge,
			.stepsTillFullyGrown=Config::stepsPerDay * data["daysTillFullyGrown"].get<uint16_t>(),
			.stepsTillDieWithoutFood=Config::stepsPerDay * data["daysTillDieWithoutFood"].get<uint16_t>(),
			.stepsEatFrequency=Config::stepsPerDay * data["daysEatFrequency"].get<uint16_t>(),
			.stepsTillDieWithoutFluid=Config::stepsPerDay * data["daysTillDieWithoutFluid"].get<uint16_t>(),
			.stepsFluidDrinkFrequency=Config::stepsPerDay * data["daysFluidDrinkFrequency"].get<uint16_t>(),
			.stepsTillDieInUnsafeTemperature=Config::stepsPerDay * data["daysTillDieInUnsafeTemperature"].get<uint16_t>(),
			.minimumSafeTemperature=data["minimumSafeTemperature"].get<Temperature>(),
			.maximumSafeTemperature=data["maximumSafeTemperature"].get<Temperature>(),
			.stepsSleepFrequency=Config::stepsPerDay * data["daysSleepFrequency"].get<uint16_t>(),
			.stepsTillSleepOveride=Config::stepsPerHour * data["hoursTillSleepOveride"].get<uint8_t>(),
			.stepsSleepDuration=Config::stepsPerHour * data["hoursSleepDuration"].get<uint8_t>(),
			.nocturnal=data["nocturnal"].get<bool>(),
			.eatsMeat=data["eatsMeat"].get<bool>(),
			.eatsLeaves=data["eatsLeaves"].get<bool>(),
			.eatsFruit=data["eatsFruit"].get<bool>(),
			.visionRange=data["visionRange"].get<Distance>(),
			.bodyScale=data["bodyScale"].get<uint32_t>(),
			.materialType=MaterialType::byName((data["materialType"].get<std::string>())),
			.moveType=MoveType::byName((data["moveType"].get<std::string>())),
			.fluidType=FluidType::byName(data["fluidType"].get<std::string>()),
			.bodyType=BodyType::byName((data["bodyType"].get<std::string>()))
		};
		assert(data.contains("shapes"));
		for(const auto& shapeName : data["shapes"])
			params.shapes.push_back(Shape::byName((shapeName.get<std::string>())));
		AnimalSpecies::create(params);
	}
}
void definitions::load()
{
	assert(std::filesystem::exists(path));
	assert(MaterialType::empty());
	loadShapes();
	loadFluidTypes();
	loadMaterialTypes();
	loadMoveTypes();
	loadSkillTypes();
	loadItemTypes();
	loadCraftJobs();
	//loadMedicalProjectTypes();
	loadPlantSpecies();
	loadBodyPartTypes();
	loadBodyTypes();
	loadAnimalSpecies();
}
