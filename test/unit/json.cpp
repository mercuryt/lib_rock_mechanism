#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/objectives/sowSeeds.h"
#include "../../engine/objectives/harvest.h"
#include "../../engine/objectives/givePlantsFluid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/dig.h"
#include "../../engine/objectives/construct.h"
#include "../../engine/objectives/stockpile.h"
#include "../../engine/portables.hpp"
#include "../../engine/numericTypes/types.h"
TEST_CASE("json")
{
	Simulation simulation{"", DateTime(12, 50, 1000).toSteps()};
	FactionId faction = simulation.createFaction("Tower of Power");
	AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	MaterialTypeId dirt = MaterialType::byName("dirt");
	MaterialTypeId wood = MaterialType::byName("poplar wood");
	MaterialTypeId cotton = MaterialType::byName("cotton");
	MaterialTypeId bronze = MaterialType::byName("bronze");
	MaterialTypeId sand = MaterialType::byName("sand");
	PlantSpeciesId sage = PlantSpecies::byName("sage brush");
	PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	FluidTypeId water = FluidType::byName("water");
	ItemTypeId axe = ItemType::byName("axe");
	ItemTypeId pick = ItemType::byName("pick");
	ItemTypeId saw = ItemType::byName("saw");
	ItemTypeId bucket = ItemType::byName("bucket");
	ItemTypeId preparedMeal = ItemType::byName("prepared meal");
	ItemTypeId pants = ItemType::byName("pants");
	ItemTypeId pile = ItemType::byName("pile");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	area.m_blockDesignations.registerFaction(faction);
	areaBuilderUtil::setSolidLayer(area, 0, dirt);
	area.m_hasFarmFields.registerFaction(faction);

	SUBCASE("basic")
	{
		Json areaData;
		Json simulationData;
		std::string name;
		{
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(area, farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(area, farm, sage);
			ActorIndex dwarf1 = actors.create({
				.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction,
				.hasCloths = false,
				.hasSidearm = false,
			});
			name = actors.getName(dwarf1);
			ObjectiveTypeId digObjectiveType = ObjectiveType::getIdByName("dig");
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(blocks.getIndex_i(9, 9, 1));
			actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(10));
			actors.objective_replaceTasks(dwarf1, std::move(objective));
			blocks.plant_create(blocks.getIndex_i(8, 8, 1), sage, Percent::create(99));
			PlantIndex sage1 = blocks.plant_get(blocks.getIndex_i(8, 8, 1));
			plants.setMaybeNeedsFluid(sage1);
			blocks.fluid_add(blocks.getIndex_i(3, 8, 1), CollisionVolume::create(10), water);
			items.create(ItemParamaters{.itemType = axe, .materialType = bronze, .location = blocks.getIndex_i(1, 2, 1), .quality = Quality::create(10), .percentWear = Percent::create(10)});
			blocks.blockFeature_construct(blocks.getIndex_i(1, 8, 1), BlockFeatureTypeId::Stairs, wood);
			blocks.blockFeature_construct(blocks.getIndex_i(9, 1, 1), BlockFeatureTypeId::Door, wood);
			area.m_fires.ignite(blocks.getIndex_i(9, 1, 1), wood);
			ItemIndex pants1 = items.create(ItemParamaters{.itemType = pants, .materialType = cotton, .quality = Quality::create(50), .percentWear = Percent::create(30)});
			actors.equipment_add(dwarf1, pants1);
			ItemIndex saw1 = items.create({.itemType = saw, .materialType = bronze, .quality = Quality::create(50), .percentWear = Percent::create(30)});
			actors.canPickUp_pickUpItem(dwarf1, saw1);
			ItemIndex bucket1 = items.create({.itemType = bucket, .materialType = bronze, .location = blocks.getIndex_i(0, 0, 1), .quality = Quality::create(50), .percentWear = Percent::create(30)});
			items.cargo_addFluid(bucket1, water, CollisionVolume::create(1));
			ItemIndex bucket2 = items.create({.itemType = bucket, .materialType = bronze, .location = blocks.getIndex_i(0, 1, 1), .quality = Quality::create(50), .percentWear = Percent::create(30)});
			items.cargo_addItemGeneric(bucket2, pile, sand, Quantity::create(1));
			// Serialize.
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		// Deserialize.
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		Items& items2 = area2.getItems();
		Plants& plants2 = area2.getPlants();
		FactionId faction2 = simulation2.m_hasFactions.byName("Tower of Power");
		// BlockIndex.
		CHECK(blocks2.m_sizeX == blocks2.m_sizeY);
		CHECK(blocks2.m_sizeX == blocks2.m_sizeZ);
		CHECK(blocks2.m_sizeX == 10);
		CHECK(blocks2.solid_is(blocks2.getIndex_i(5,5,0)));
		CHECK(blocks2.solid_get(blocks2.getIndex_i(5,5,0)) == dirt);
		// Plant.
		CHECK(blocks2.plant_exists(blocks2.getIndex_i(8,8,1)));
		PlantIndex sage2 = blocks2.plant_get(blocks2.getIndex_i(8,8,1));
		CHECK(plants2.getSpecies(sage2) == sage);
		CHECK(plants2.getPercentGrown(sage2) == 99);
		CHECK(plants2.getStepAtWhichPlantWillDieFromLackOfFluid(sage2) != 0);
		CHECK(plants2.getStepAtWhichPlantWillDieFromLackOfFluid(sage2).exists());
		CHECK(plants2.fluidEventExists(sage2));
		CHECK(!plants2.growthEventExists(sage2));
		// Fluid.
		BlockIndex waterLocation = blocks2.getIndex_i(3,8,1);
		CHECK(blocks2.fluid_getTotalVolume(waterLocation) == 10);
		CHECK(blocks2.fluid_volumeOfTypeContains(waterLocation, water) == 10);
		FluidGroup& fluidGroup = *blocks2.fluid_getGroup(waterLocation, water);
		CHECK(area2.m_hasFluidGroups.getUnstable().contains(&fluidGroup));
		// Actor.
		CHECK(!blocks2.actor_empty(blocks2.getIndex_i(5,5,1)));
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];
		CHECK(actors2.getSpecies(dwarf2) == dwarf);
		CHECK(actors2.getPercentGrown(dwarf2) == 90);
		CHECK(actors2.getName(dwarf2) == name);
		CHECK(actors2.getFaction(dwarf2) == faction2);
		CHECK(actors2.eat_hasHungerEvent(dwarf2));
		CHECK(actors2.drink_hasThristEvent(dwarf2));
		CHECK(actors2.sleep_hasTiredEvent(dwarf2));
		// Objective.
		CHECK(actors2.objective_getPriorityFor(dwarf2, ObjectiveType::getIdByName("dig")) == 10);
		CHECK(actors2.objective_getCurrentName(dwarf2) == "go to");
		GoToObjective& goToObjective = actors2.objective_getCurrent<GoToObjective>(dwarf2);
		CHECK(goToObjective.getLocation() == blocks2.getIndex_i(9,9,1));
		// Equipment.
		CHECK(!actors2.equipment_getAll(dwarf2).empty());
		ItemIndex pants2 = actors2.equipment_getAll(dwarf2).begin()->getIndex(items2.m_referenceData);
		CHECK(items2.getItemType(pants2) == pants);
		CHECK(items2.getMaterialType(pants2) == cotton);
		// canPickup.
		CHECK(actors2.canPickUp_exists(dwarf2));
		CHECK(actors2.canPickUp_getCarrying(dwarf2).isItem());
		ItemIndex saw2 = actors2.canPickUp_getItem(dwarf2);
		CHECK(items2.getItemType(saw2) == saw);
		CHECK(items2.getMaterialType(saw2) == bronze);
		// items
		CHECK(!blocks2.item_empty(blocks2.getIndex_i(1,2,1)));
		ItemIndex axe2 = blocks2.item_getAll(blocks2.getIndex_i(1,2,1))[0];
		CHECK(items2.getMaterialType(axe2) == bronze);
		CHECK(items2.getQuality(axe2) == 10);
		CHECK(items2.getWear(axe2) == 10);
		// BlockIndex features.
		CHECK(blocks2.blockFeature_contains(blocks2.getIndex_i(1,8,1), BlockFeatureTypeId::Stairs));
		CHECK(blocks2.blockFeature_atConst(blocks2.getIndex_i(1,8,1), BlockFeatureTypeId::Stairs)->materialType == wood);
		CHECK(blocks2.blockFeature_contains(blocks2.getIndex_i(9,1,1), BlockFeatureTypeId::Door));
		// Item cargo.
		CHECK(!blocks2.item_empty(blocks2.getIndex_i(0,0,1)));
		CHECK(blocks2.item_getAll(blocks2.getIndex_i(0,0,1)).size() == 1);
		ItemIndex bucket3 = blocks2.item_getAll(blocks2.getIndex_i(0,0,1))[0];
		CHECK(bucket3.exists());
		CHECK(items2.cargo_containsAnyFluid(bucket3));
		CHECK(items2.cargo_getFluidType(bucket3) == water);
		CHECK(items2.cargo_getFluidVolume(bucket3) == 1);
		CHECK(blocks2.item_getAll(blocks2.getIndex_i(0,1,1)).size() == 1);
		ItemIndex bucket4 = blocks2.item_getAll(blocks2.getIndex_i(0,1,1))[0];
		CHECK(bucket4.exists());
		CHECK(items2.cargo_containsItemGeneric(bucket4, pile, sand, Quantity::create(1)));
		// Fires.
		CHECK(area2.m_fires.contains(blocks2.getIndex_i(9,1,1), wood));
		Fire& fire = area2.m_fires.at(blocks2.getIndex_i(9,1,1), wood);
		CHECK(fire.m_stage == FireStage::Smouldering);
		CHECK(fire.m_hasPeaked == false);
		CHECK(fire.m_event.exists());
		// Farm fields.
		CHECK(area2.m_hasFarmFields.contains(faction2));
		CHECK(blocks2.farm_contains(blocks2.getIndex_i(1,6,1), faction2));
		CHECK(blocks2.farm_get(blocks2.getIndex_i(1,6,1), faction2)->plantSpecies == sage);
		CHECK(blocks2.farm_contains(blocks2.getIndex_i(1,7,1), faction2));
		// OpacityFacade.
		area2.m_opacityFacade.validate();
	}
	SUBCASE("dig project")
	{
		Json areaData;
		Json simulationData;
		{
			BlockIndex holeLocation = blocks.getIndex_i(8, 4, 0);
			area.m_hasDigDesignations.addFaction(faction);
			area.m_hasDigDesignations.designate(faction, holeLocation, BlockFeatureTypeId::Null);
			items.create({.itemType = pick, .materialType = bronze, .location = blocks.getIndex_i(1, 2, 1), .quality = Quality::create(10), .percentWear = Percent::create(10)});
			ActorIndex dwarf1 = actors.create({
				.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction,
			});
			ObjectiveTypeId digObjectiveType = ObjectiveType::getIdByName("dig");
			actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
			// One step to find the designation.
			simulation.doStep();
			// One step to activate the project and reserve the pick.
			simulation.doStep();
			// One step to select haul type.
			simulation.doStep();
			// Another step to path to the pick.
			simulation.doStep();

			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		Items& items2 = area2.getItems();
		FactionId faction2 = simulation2.m_hasFactions.byName("Tower of Power");
		BlockIndex holeLocation2 = blocks2.getIndex_i(8,4,0);
		CHECK(area2.m_hasDigDesignations.contains(faction2, holeLocation2));
		DigProject& project = area2.m_hasDigDesignations.getForFactionAndBlock(faction2, holeLocation2);
		CHECK(project.getWorkersAndCandidates().size() == 1);
		CHECK(project.getWorkers().size() == 1);
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];
		CHECK(actors2.project_get(dwarf2) == static_cast<Project*>(&project));
		CHECK(!actors2.move_getPath(dwarf2).empty());
		DigObjective& objective = actors2.objective_getCurrent<DigObjective>(dwarf2);
		CHECK(objective.getProject() == actors2.project_get(dwarf2));
		ActorReference dwarf2Ref = area2.getActors().m_referenceData.getReference(dwarf2);
		const ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf2Ref);
		CHECK(projectWorker.haulSubproject);
		CHECK(projectWorker.objective == &actors2.objective_getCurrent<Objective>(dwarf2));
		HaulSubproject& haulSubproject = *projectWorker.haulSubproject;
		CHECK(haulSubproject.getWorkers().contains(dwarf2Ref));
		CHECK(haulSubproject.getHaulStrategy() == HaulStrategy::Individual);
		ItemIndex pick2 = blocks2.item_getAll(blocks2.getIndex_i(1,2,1))[0];
		CHECK(haulSubproject.getToHaul().isItem());
		CHECK(haulSubproject.getToHaul().getIndex(actors2.m_referenceData, items2.m_referenceData).toItem() == pick2);
		CHECK(haulSubproject.getQuantity() == 1);
		CHECK(!haulSubproject.getIsMoving());
		CHECK(!project.hasTryToHaulEvent());
		CHECK(!project.hasTryToHaulThreadedTask());
		CHECK(!project.hasTryToReserveEvent());
		CHECK(!project.hasTryToAddWorkersThreadedTask());
	}
	SUBCASE("construct project")
	{
		Json areaData;
		Json simulationData;
		{
			BlockIndex projectLocation = blocks.getIndex_i(8, 4, 1);
			area.m_hasConstructionDesignations.addFaction(faction);
			area.m_hasConstructionDesignations.designate(faction, projectLocation, BlockFeatureTypeId::Null, wood);
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		FactionId faction2 = simulation2.m_hasFactions.byName("Tower of Power");
		BlockIndex projectLocation2 = blocks2.getIndex_i(8,4,1);
		CHECK(area2.m_hasConstructionDesignations.contains(faction2, projectLocation2));
		ConstructProject& project = area2.m_hasConstructionDesignations.getProject(faction2, projectLocation2);
		CHECK(project.getMaterialType() == wood);
		CHECK(project.getWorkers().size() == 0);
		CHECK(!project.hasTryToHaulEvent());
		CHECK(!project.hasTryToHaulThreadedTask());
		CHECK(!project.hasTryToReserveEvent());
	}
	SUBCASE("stockpile project")
	{
		Json areaData;
		Json simulationData;
		{
			BlockIndex projectLocation1 = blocks.getIndex_i(8, 4, 1);
			BlockIndex pileLocation1 = blocks.getIndex_i(1, 4, 1);
			area.m_hasStockPiles.registerFaction(faction);
			std::vector<ItemQuery> queries;
			queries.emplace_back(ItemQuery::create(pile));
			StockPile &stockPile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
			stockPile.addBlock(projectLocation1);
			ItemIndex pile1 = items.create({.itemType = pile, .materialType = dirt, .location = pileLocation1, .quantity = Quantity::create(10)});
			CHECK(stockPile.accepts(pile1));
			area.m_hasStockPiles.getForFaction(faction).addItem(pile1);
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction
			});
			ObjectiveTypeId stockPileObjectiveType = ObjectiveType::getIdByName("stockpile");
			actors.objective_setPriority(dwarf1, stockPileObjectiveType, Priority::create(100));
			// One step to find the pile.
			simulation.doStep();
			// One step to confirm the destaination.
			simulation.doStep();
			CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithProjectsCount() == 1);

			areaData = area.toJson();
			simulationData = simulation.toJson();
			CHECK(!areaData["hasStockPiles"].empty());
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		Items& items2 = area2.getItems();
		FactionId faction2 = simulation2.m_hasFactions.byName("Tower of Power");
		BlockIndex projectLocation2 = blocks2.getIndex_i(8,4,1);
		BlockIndex pileLocation2 = blocks2.getIndex_i(1, 4, 1);
		ItemIndex pile2 = blocks2.item_getAll(pileLocation2)[0];
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];
		CHECK(blocks2.stockpile_contains(projectLocation2, faction2));
		StockPile& stockPile2 = *blocks2.stockpile_getForFaction(projectLocation2, faction2);
		CHECK(stockPile2.accepts(pile2));
		CHECK(area2.m_hasStockPiles.getForFaction(faction2).getItemsWithProjectsCount() == 1);
		StockPileObjective& objective = actors2.objective_getCurrent<StockPileObjective>(dwarf2);
		CHECK(objective.m_project != nullptr);
		CHECK(actors2.project_exists(dwarf2));
		StockPileProject& project = *objective.m_project;
		CHECK(project.getWorkers().size() == 1);
		CHECK(actors2.project_get(dwarf2) == static_cast<Project*>(&project));
		CHECK(actors2.move_getPath(dwarf2).empty());
		CHECK(project.getItem().getIndex(items2.m_referenceData) == pile2);
		CHECK(project.getLocation() == projectLocation2);
	}
	SUBCASE("craft")
	{
		Json areaData;
		Json simulationData;
		CraftStepTypeCategoryId craftStepTypeSaw = CraftStepTypeCategory::byName("saw");
		CraftStepTypeCategoryId craftStepTypeScrape = CraftStepTypeCategory::byName("scrape");
		CraftJobTypeId jobTypeSawBoards = CraftJobType::byName("saw boards");
		{
			area.m_hasCraftingLocationsAndJobs.addFaction(faction);
			area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(craftStepTypeSaw, blocks.getIndex_i(3, 3, 1));
			area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addLocation(craftStepTypeScrape, blocks.getIndex_i(3, 2, 1));
			area.m_hasCraftingLocationsAndJobs.getForFaction(faction).addJob(jobTypeSawBoards, wood, Quantity::create(1));
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction}
			);
			ObjectiveTypeId woodWorkingObjectiveType = ObjectiveType::getIdByName("craft: wood working");
			actors.objective_setPriority(dwarf1, woodWorkingObjectiveType, Priority::create(100));
			items.create({.itemType = ItemType::byName("saw"), .materialType = bronze, .location = blocks.getIndex_i(3, 7, 1), .quality = Quality::create(25), .percentWear = Percent::create(0)});
			items.create({.itemType = ItemType::byName("chisel"), .materialType = bronze, .location = blocks.getIndex_i(3, 6, 1), .quality = Quality::create(25), .percentWear = Percent::create(0)});
			items.create({.itemType = ItemType::byName("log"), .materialType = wood, .location = blocks.getIndex_i(3, 8, 1), .quantity = Quantity::create(1)});
			// One step to find the designation.
			simulation.doStep();

			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		FactionId faction2 = simulation2.m_hasFactions.byName("Tower of Power");
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];

		CHECK(area2.m_hasCraftingLocationsAndJobs.getForFaction(faction2).hasLocationsForCategory(craftStepTypeSaw));
		CHECK(area2.m_hasCraftingLocationsAndJobs.getForFaction(faction2).hasLocationsForCategory(craftStepTypeScrape));
		CHECK(area2.m_hasCraftingLocationsAndJobs.getForFaction(faction2).hasJobs());
		CHECK(actors2.project_exists(dwarf2));
		CHECK(actors2.objective_getCurrent<CraftObjective>(dwarf2).getCraftJob() != nullptr);
		CraftJob& job = *actors2.objective_getCurrent<CraftObjective>(dwarf2).getCraftJob();
		CHECK(job.craftStepProject);
		CHECK(job.craftJobType == jobTypeSawBoards);
		CHECK(job.materialType == wood);
		CraftStepProject& project = *job.craftStepProject.get();
		CHECK(actors2.project_get(dwarf2) == &project);
	}
	SUBCASE("drink")
	{
		Json areaData;
		Json simulationData;
		{
			ItemIndex bucket1 = items.create({.itemType = bucket, .materialType = bronze, .location = blocks.getIndex_i(3, 7, 1), .quality = Quality::create(25), .percentWear = Percent::create(0)});
			items.cargo_addFluid(bucket1, water, CollisionVolume::create(2));
			ActorIndex dwarf1 = actors.create({
				.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
			});
			actors.drink_setNeedsFluid(dwarf1);
			// One step to find the bucket.
			simulation.doStep();
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];

		CHECK(actors2.move_getDestination(dwarf2).exists());
		CHECK(blocks2.isAdjacentToIncludingCornersAndEdges(actors2.move_getDestination(dwarf2), blocks2.getIndex_i(3,7,1)));
		CHECK(actors2.objective_getCurrent<Objective>(dwarf2).getTypeId() == ObjectiveType::getIdByName("drink"));
	}
	SUBCASE("eat")
	{
		Json areaData;
		Json simulationData;
		{
			items.create({
				.itemType = preparedMeal,
				.materialType = MaterialType::byName("fruit"),
				.location = blocks.getIndex_i(3, 7, 1),
				.quality = Quality::create(25),
				.percentWear = Percent::create(0),
			});
			ActorIndex dwarf1 = actors.create({
				.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
			});
			actors.eat_setIsHungry(dwarf1);
			// One step to find the meal.
			simulation.doStep();
			// Serialize
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];

		CHECK(actors2.move_getDestination(dwarf2).exists());
		CHECK(blocks2.isAdjacentToIncludingCornersAndEdges(actors2.move_getDestination(dwarf2), blocks2.getIndex_i(3,7,1)));
		CHECK(actors2.objective_getCurrent<Objective>(dwarf2).getTypeId() == ObjectiveType::getIdByName("eat"));
	}
	SUBCASE("sleep")
	{
		Json areaData;
		Json simulationData;
		ActorId dwarf1Id;
		{
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction
			});
			area.m_hasSleepingSpots.designate(faction, blocks.getIndex_i(2, 2, 1));
			area.m_hasSleepingSpots.designate(faction, blocks.getIndex_i(1, 1, 1));
			actors.sleep_makeTired(dwarf1);
			// Discard drink objective if exists.
			if (actors.drink_getVolumeOfFluidRequested(dwarf1) != 0)
				actors.drink_do(dwarf1, actors.drink_getVolumeOfFluidRequested(dwarf1));
			if (actors.eat_getMassFoodRequested(dwarf1) != 0)
				actors.eat_do(dwarf1, actors.eat_getMassFoodRequested(dwarf1));
			dwarf1Id = actors.getId(dwarf1);

			// One step to find the sleeping spot.
			simulation.doStep();
			// Serialize.
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		ActorIndex dwarf2 = simulation2.m_actors.getIndexForId(dwarf1Id);

		CHECK(actors2.sleep_hasTiredEvent(dwarf2));
		CHECK(actors2.sleep_getPercentDoneSleeping(dwarf2) == 0);
		CHECK(area2.m_hasSleepingSpots.containsUnassigned(blocks2.getIndex_i(1,1,1)));
		CHECK(actors2.move_getDestination(dwarf2).exists());
		CHECK(actors2.move_getDestination(dwarf2) == blocks2.getIndex_i(2,2,1));
		CHECK(actors2.objective_getCurrent<Objective>(dwarf2).getTypeId() == ObjectiveType::getIdByName("sleep"));
		CHECK(actors2.sleep_getSpot(dwarf2).exists());
	}
	SUBCASE("sow seed")
	{
		Json areaData;
		Json simulationData;
		ActorId dwarf1Id;
		Point3D coordinates1;
		{
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(area, farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(area, farm, sage);
			ActorIndex dwarf1 = actors.create({
				.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction,
			});
			ObjectiveTypeId sowObjectiveType = ObjectiveType::getIdByName("sow seeds");
			actors.objective_setPriority(dwarf1, sowObjectiveType, Priority::create(10));
			dwarf1Id = actors.getId(dwarf1);

			// One step to find the sowing location.
			simulation.doStep();

			SowSeedsObjective &objective = actors.objective_getCurrent<SowSeedsObjective>(dwarf1);
			BlockIndex block1 = objective.getBlock();
			coordinates1 = area.getBlocks().getCoordinates(block1);
			// Serialize.
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		ActorIndex dwarf2 = simulation2.m_actors.getIndexForId(dwarf1Id);

		Objective& objective2 = actors2.objective_getCurrent<Objective>(dwarf2);
		CHECK(objective2.getTypeId() == ObjectiveType::getIdByName("sow seeds"));
		CHECK(actors2.move_getDestination(dwarf2).exists());
		BlockIndex block2 = static_cast<SowSeedsObjective&>(objective2).getBlock();
		Point3D coordinates2 = blocks2.getCoordinates(block2);
		CHECK(coordinates2.x() == coordinates1.x());
		CHECK(coordinates2.y() == coordinates1.y());
		CHECK(coordinates2.z() == coordinates1.z());
	}
	SUBCASE("harvest")
	{
		Json areaData;
		Json simulationData;
		ActorId dwarf1Id;
		{
			blocks.plant_create(blocks.getIndex_i(1, 7, 1), wheatGrass, Percent::create(100));
			PlantIndex plant = blocks.plant_get(blocks.getIndex_i(1, 7, 1));
			plants.setQuantityToHarvest(plant);
			CHECK(plants.getQuantityToHarvest(plant) != 0);
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(area, farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(area, farm, wheatGrass);
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction}
			);
			ObjectiveTypeId harvestObjectiveType = ObjectiveType::getIdByName("harvest");
			actors.objective_setPriority(dwarf1, harvestObjectiveType, Priority::create(10));
			dwarf1Id = actors.getId(dwarf1);
			// One step to find the harvest location.
			simulation.doStep();
			// Serialize.
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		ActorIndex dwarf2 = simulation2.m_actors.getIndexForId(dwarf1Id);
		Objective& objective2 = actors2.objective_getCurrent<Objective>(dwarf2);
		CHECK(objective2.getTypeId() == ObjectiveType::getIdByName("harvest"));
		HarvestObjective& harvestObjective = static_cast<HarvestObjective&>(objective2);
		CHECK(harvestObjective.getBlock() == blocks2.getIndex_i(1,7,1));
	}
	SUBCASE("give fluid to plant")
	{
		Json areaData;
		Json simulationData;
		ActorId dwarf1Id;
		{
			ItemIndex bucket1 = items.create({.itemType = bucket, .materialType = bronze, .location = blocks.getIndex_i(3, 7, 1), .quality = Quality::create(25), .percentWear = Percent::create(0)});
			items.cargo_addFluid(bucket1, water, CollisionVolume::create(2));
			blocks.plant_create(blocks.getIndex_i(1, 7, 1), wheatGrass, Percent::create(100));
			PlantIndex plant = blocks.plant_get(blocks.getIndex_i(1, 7, 1));
			plants.setMaybeNeedsFluid(plant);
			CHECK(plants.getVolumeFluidRequested(plant) != 0);
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(area, farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(area, farm, wheatGrass);
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction
			});
			ObjectiveTypeId givePlantsFluidObjectiveType = ObjectiveType::getIdByName("give plants fluid");
			actors.objective_setPriority(dwarf1, givePlantsFluidObjectiveType, Priority::create(10));
			CHECK(actors.objective_getCurrentName(dwarf1) == "give plants fluid");
			dwarf1Id = actors.getId(dwarf1);
			// One step to find the plant needing water.
			simulation.doStep();
			CHECK(actors.objective_getCurrentName(dwarf1) == "give plants fluid");
			// Serialize.
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Actors& actors2 = area2.getActors();
		ActorIndex dwarf2 = simulation2.m_actors.getIndexForId(dwarf1Id);
		Objective& objective2 = actors2.objective_getCurrent<Objective>(dwarf2);
		CHECK(objective2.name() == "give plants fluid");
	}
}