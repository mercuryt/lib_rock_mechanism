#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
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
#include "objectives/construct.h"
#include "objectives/stockpile.h"
#include "types.h"
TEST_CASE("json")
{
	Simulation simulation{L"", DateTime(12, 50, 1000).toSteps()};
	FactionId faction = simulation.createFaction(L"Tower of Power");
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
		std::wstring name;
		{
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(farm, sage);
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
			blocks.blockFeature_construct(blocks.getIndex_i(1, 8, 1), BlockFeatureType::stairs, wood);
			blocks.blockFeature_construct(blocks.getIndex_i(9, 1, 1), BlockFeatureType::door, wood);
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
		FactionId faction2 = simulation2.m_hasFactions.byName(L"Tower of Power");
		// BlockIndex.
		REQUIRE(blocks2.m_sizeX == blocks2.m_sizeY);
		REQUIRE(blocks2.m_sizeX == blocks2.m_sizeZ);
		REQUIRE(blocks2.m_sizeX == 10);
		REQUIRE(blocks2.solid_is(blocks2.getIndex_i(5,5,0)));
		REQUIRE(blocks2.solid_get(blocks2.getIndex_i(5,5,0)) == dirt);
		// Plant.
		REQUIRE(blocks2.plant_exists(blocks2.getIndex_i(8,8,1)));
		PlantIndex sage2 = blocks2.plant_get(blocks2.getIndex_i(8,8,1));
		REQUIRE(plants2.getSpecies(sage2) == sage);
		REQUIRE(plants2.getPercentGrown(sage2) == 99);
		REQUIRE(plants2.getStepAtWhichPlantWillDieFromLackOfFluid(sage2) != 0);
		REQUIRE(plants2.getStepAtWhichPlantWillDieFromLackOfFluid(sage2).exists());
		REQUIRE(plants2.fluidEventExists(sage2));
		REQUIRE(!plants2.growthEventExists(sage2));
		// Fluid.
		BlockIndex waterLocation = blocks2.getIndex_i(3,8,1);
		REQUIRE(blocks2.fluid_getTotalVolume(waterLocation) == 10);
		REQUIRE(blocks2.fluid_volumeOfTypeContains(waterLocation, water) == 10);
		FluidGroup& fluidGroup = *blocks2.fluid_getGroup(waterLocation, water);
		REQUIRE(area2.m_hasFluidGroups.getUnstable().contains(&fluidGroup));
		// Actor.
		REQUIRE(!blocks2.actor_empty(blocks2.getIndex_i(5,5,1)));
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];
		REQUIRE(actors2.getSpecies(dwarf2) == dwarf);
		REQUIRE(actors2.getPercentGrown(dwarf2) == 90);
		REQUIRE(actors2.getName(dwarf2) == name);
		REQUIRE(actors2.getFaction(dwarf2) == faction2);
		REQUIRE(actors2.eat_hasHungerEvent(dwarf2));
		REQUIRE(actors2.drink_hasThristEvent(dwarf2));
		REQUIRE(actors2.sleep_hasTiredEvent(dwarf2));
		// Objective.
		REQUIRE(actors2.objective_getPriorityFor(dwarf2, ObjectiveType::getIdByName("dig")) == 10);
		REQUIRE(actors2.objective_getCurrentName(dwarf2) == "go to");
		GoToObjective& goToObjective = actors2.objective_getCurrent<GoToObjective>(dwarf2);
		REQUIRE(goToObjective.getLocation() == blocks2.getIndex_i(9,9,1));
		// Equipment.
		REQUIRE(!actors2.equipment_getAll(dwarf2).empty());
		ItemIndex pants2 = actors2.equipment_getAll(dwarf2).begin()->getIndex();
		REQUIRE(items2.getItemType(pants2) == pants);
		REQUIRE(items2.getMaterialType(pants2) == cotton);
		// canPickup.
		REQUIRE(actors2.canPickUp_exists(dwarf2));
		REQUIRE(actors2.canPickUp_getCarrying(dwarf2).isItem());
		ItemIndex saw2 = actors2.canPickUp_getItem(dwarf2);
		REQUIRE(items2.getItemType(saw2) == saw);
		REQUIRE(items2.getMaterialType(saw2) == bronze);
		// items2
		REQUIRE(!blocks2.item_empty(blocks2.getIndex_i(1,2,1)));
		ItemIndex axe2 = blocks2.item_getAll(blocks2.getIndex_i(1,2,1))[0];
		REQUIRE(items2.getMaterialType(axe2) == bronze);
		REQUIRE(items2.getQuality(axe2) == 10);
		REQUIRE(items2.getWear(axe2) == 10);
		// BlockIndex features.
		REQUIRE(blocks2.blockFeature_contains(blocks2.getIndex_i(1,8,1), BlockFeatureType::stairs));
		REQUIRE(blocks2.blockFeature_atConst(blocks2.getIndex_i(1,8,1), BlockFeatureType::stairs)->materialType == wood);
		REQUIRE(blocks2.blockFeature_contains(blocks2.getIndex_i(9,1,1), BlockFeatureType::door));
		// Item cargo.
		REQUIRE(!blocks2.item_empty(blocks2.getIndex_i(0,0,1)));
		REQUIRE(blocks2.item_getAll(blocks2.getIndex_i(0,0,1)).size() == 1);
		ItemIndex bucket3 = blocks2.item_getAll(blocks2.getIndex_i(0,0,1))[0];
		REQUIRE(bucket3.exists());
		REQUIRE(items2.cargo_containsAnyFluid(bucket3));
		REQUIRE(items2.cargo_getFluidType(bucket3) == water);
		REQUIRE(items2.cargo_getFluidVolume(bucket3) == 1);
		REQUIRE(blocks2.item_getAll(blocks2.getIndex_i(0,1,1)).size() == 1);
		ItemIndex bucket4 = blocks2.item_getAll(blocks2.getIndex_i(0,1,1))[0];
		REQUIRE(bucket4.exists());
		REQUIRE(items2.cargo_containsItemGeneric(bucket4, pile, sand, Quantity::create(1)));
		// Fires.
		REQUIRE(area2.m_fires.contains(blocks2.getIndex_i(9,1,1), wood));
		Fire& fire = area2.m_fires.at(blocks2.getIndex_i(9,1,1), wood);
		REQUIRE(fire.m_stage == FireStage::Smouldering);
		REQUIRE(fire.m_hasPeaked == false);
		REQUIRE(fire.m_event.exists());
		// Farm fields.
		REQUIRE(area2.m_hasFarmFields.contains(faction2));
		REQUIRE(blocks2.farm_contains(blocks2.getIndex_i(1,6,1), faction2));
		REQUIRE(blocks2.farm_get(blocks2.getIndex_i(1,6,1), faction2)->plantSpecies == sage);
		REQUIRE(blocks2.farm_contains(blocks2.getIndex_i(1,7,1), faction2));
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
			area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
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
		FactionId faction2 = simulation2.m_hasFactions.byName(L"Tower of Power");
		BlockIndex holeLocation2 = blocks2.getIndex_i(8,4,0);
		REQUIRE(area2.m_hasDigDesignations.contains(faction2, holeLocation2));
		DigProject& project = area2.m_hasDigDesignations.getForFactionAndBlock(faction2, holeLocation2);
		REQUIRE(project.getWorkersAndCandidates().size() == 1);
		REQUIRE(project.getWorkers().size() == 1);
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];
		REQUIRE(actors2.project_get(dwarf2) == static_cast<Project*>(&project));
		REQUIRE(!actors2.move_getPath(dwarf2).empty());
		DigObjective& objective = actors2.objective_getCurrent<DigObjective>(dwarf2);
		REQUIRE(objective.getProject() == actors2.project_get(dwarf2));
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf2.toReference(area2));
		REQUIRE(projectWorker.haulSubproject);
		REQUIRE(projectWorker.objective == &actors2.objective_getCurrent<Objective>(dwarf2));
		HaulSubproject& haulSubproject = *projectWorker.haulSubproject;
		REQUIRE(haulSubproject.getWorkers().contains(dwarf2));
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Individual);
		ItemIndex pick2 = blocks2.item_getAll(blocks2.getIndex_i(1,2,1))[0];
		REQUIRE(haulSubproject.getToHaul().isItem());
		REQUIRE(haulSubproject.getToHaul().getIndex().toItem() == pick2);
		REQUIRE(haulSubproject.getQuantity() == 1);
		REQUIRE(!haulSubproject.getIsMoving());
		REQUIRE(!project.hasTryToHaulEvent());
		REQUIRE(!project.hasTryToHaulThreadedTask());
		REQUIRE(!project.hasTryToReserveEvent());
		REQUIRE(!project.hasTryToAddWorkersThreadedTask());
	}
	SUBCASE("construct project")
	{
		Json areaData;
		Json simulationData;
		{
			BlockIndex projectLocation = blocks.getIndex_i(8, 4, 1);
			area.m_hasConstructionDesignations.addFaction(faction);
			area.m_hasConstructionDesignations.designate(faction, projectLocation, nullptr, wood);
			areaData = area.toJson();
			simulationData = simulation.toJson();
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		FactionId faction2 = simulation2.m_hasFactions.byName(L"Tower of Power");
		BlockIndex projectLocation2 = blocks2.getIndex_i(8,4,1);
		REQUIRE(area2.m_hasConstructionDesignations.contains(faction2, projectLocation2));
		ConstructProject& project = area2.m_hasConstructionDesignations.getProject(faction2, projectLocation2);
		REQUIRE(project.getMaterialType() == wood);
		REQUIRE(project.getWorkers().size() == 0);
		REQUIRE(!project.hasTryToHaulEvent());
		REQUIRE(!project.hasTryToHaulThreadedTask());
		REQUIRE(!project.hasTryToReserveEvent());
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
			queries.emplace_back(pile);
			StockPile &stockPile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
			stockPile.addBlock(projectLocation1);
			ItemIndex pile1 = items.create({.itemType = pile, .materialType = dirt, .location = pileLocation1, .quantity = Quantity::create(10)});
			REQUIRE(stockPile.accepts(pile1));
			area.m_hasStockPiles.getForFaction(faction).addItem(pile1);
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction}
			);
			ObjectiveTypeId stockPileObjectiveType = ObjectiveType::getIdByName("stockpile");
			actors.objective_setPriority(dwarf1, stockPileObjectiveType, Priority::create(100));
			// One step to find the pile.
			simulation.doStep();
			// One step to confirm the destaination.
			simulation.doStep();
			REQUIRE(area.m_hasStockPiles.getForFaction(faction).getItemsWithProjectsCount() == 1);

			areaData = area.toJson();
			simulationData = simulation.toJson();
			REQUIRE(!areaData["hasStockPiles"].empty());
		}
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actors& actors2 = area2.getActors();
		FactionId faction2 = simulation2.m_hasFactions.byName(L"Tower of Power");
		BlockIndex projectLocation2 = blocks2.getIndex_i(8,4,1);
		BlockIndex pileLocation2 = blocks2.getIndex_i(1, 4, 1);
		ItemIndex pile2 = blocks2.item_getAll(pileLocation2)[0];
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];
		REQUIRE(blocks2.stockpile_contains(projectLocation2, faction2));
		StockPile& stockPile2 = *blocks2.stockpile_getForFaction(projectLocation2, faction2);
		REQUIRE(stockPile2.accepts(pile2));
		REQUIRE(area2.m_hasStockPiles.getForFaction(faction2).getItemsWithProjectsCount() == 1);
		StockPileObjective& objective = actors2.objective_getCurrent<StockPileObjective>(dwarf2);
		REQUIRE(objective.m_project != nullptr);
		REQUIRE(actors2.project_exists(dwarf2));
		StockPileProject& project = *objective.m_project;
		REQUIRE(project.getWorkers().size() == 1);
		REQUIRE(actors2.project_get(dwarf2) == static_cast<Project*>(&project));
		REQUIRE(actors2.move_getPath(dwarf2).empty());
		REQUIRE(project.getItem() == pile2);
		REQUIRE(project.getLocation() == projectLocation2);
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
		FactionId faction2 = simulation2.m_hasFactions.byName(L"Tower of Power");
		ActorIndex dwarf2 = blocks2.actor_getAll(blocks2.getIndex_i(5,5,1))[0];

		REQUIRE(area2.m_hasCraftingLocationsAndJobs.getForFaction(faction2).hasLocationsForCategory(craftStepTypeSaw));
		REQUIRE(area2.m_hasCraftingLocationsAndJobs.getForFaction(faction2).hasLocationsForCategory(craftStepTypeScrape));
		REQUIRE(area2.m_hasCraftingLocationsAndJobs.getForFaction(faction2).hasJobs());
		REQUIRE(actors2.project_exists(dwarf2));
		REQUIRE(actors2.objective_getCurrent<CraftObjective>(dwarf2).getCraftJob() != nullptr);
		CraftJob& job = *actors2.objective_getCurrent<CraftObjective>(dwarf2).getCraftJob();
		REQUIRE(job.craftStepProject);
		REQUIRE(job.craftJobType == jobTypeSawBoards);
		REQUIRE(job.materialType == wood);
		CraftStepProject& project = *job.craftStepProject.get();
		REQUIRE(actors2.project_get(dwarf2) == &project);
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

		REQUIRE(actors2.move_getDestination(dwarf2).exists());
		REQUIRE(blocks2.isAdjacentToIncludingCornersAndEdges(actors2.move_getDestination(dwarf2), blocks2.getIndex_i(3,7,1)));
		REQUIRE(actors2.objective_getCurrent<Objective>(dwarf2).getTypeId() == ObjectiveType::getIdByName("drink"));
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

		REQUIRE(actors2.move_getDestination(dwarf2).exists());
		REQUIRE(blocks2.isAdjacentToIncludingCornersAndEdges(actors2.move_getDestination(dwarf2), blocks2.getIndex_i(3,7,1)));
		REQUIRE(actors2.objective_getCurrent<Objective>(dwarf2).getTypeId() == ObjectiveType::getIdByName("eat"));
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

		REQUIRE(actors2.sleep_hasTiredEvent(dwarf2));
		REQUIRE(actors2.sleep_getPercentDoneSleeping(dwarf2) == 0);
		REQUIRE(area2.m_hasSleepingSpots.containsUnassigned(blocks2.getIndex_i(1,1,1)));
		REQUIRE(actors2.move_getDestination(dwarf2).exists());
		REQUIRE(actors2.move_getDestination(dwarf2) == blocks2.getIndex_i(2,2,1));
		REQUIRE(actors2.objective_getCurrent<Objective>(dwarf2).getTypeId() == ObjectiveType::getIdByName("sleep"));
		REQUIRE(actors2.sleep_getSpot(dwarf2).exists());
	}
	SUBCASE("sow seed")
	{
		Json areaData;
		Json simulationData;
		ActorId dwarf1Id;
		Point3D coordinates1;
		{
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(farm, sage);
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
		REQUIRE(objective2.getTypeId() == ObjectiveType::getIdByName("sow seeds"));
		REQUIRE(actors2.move_getDestination(dwarf2).exists());
		BlockIndex block2 = static_cast<SowSeedsObjective&>(objective2).getBlock();
		Point3D coordinates2 = blocks2.getCoordinates(block2);
		REQUIRE(coordinates2.x == coordinates1.x);
		REQUIRE(coordinates2.y == coordinates1.y);
		REQUIRE(coordinates2.z == coordinates1.z);
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
			REQUIRE(plants.getQuantityToHarvest(plant) != 0);
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(farm, wheatGrass);
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
		REQUIRE(objective2.getTypeId() == ObjectiveType::getIdByName("harvest"));
		HarvestObjective& harvestObjective = static_cast<HarvestObjective&>(objective2);
		REQUIRE(harvestObjective.getBlock() == blocks2.getIndex_i(1,7,1));
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
			REQUIRE(plants.getVolumeFluidRequested(plant) != 0);
			Cuboid farmBlocks{blocks, blocks.getIndex_i(1, 7, 1), blocks.getIndex_i(1, 6, 1)};
			FarmField &farm = area.m_hasFarmFields.getForFaction(faction).create(farmBlocks);
			area.m_hasFarmFields.getForFaction(faction).setSpecies(farm, wheatGrass);
			ActorIndex dwarf1 = actors.create({.species = dwarf,
				.percentGrown = Percent::create(90),
				.location = blocks.getIndex_i(5, 5, 1),
				.faction = faction
			});
			ObjectiveTypeId harvestObjectiveType = ObjectiveType::getIdByName("give plants fluid");
			actors.objective_setPriority(dwarf1, harvestObjectiveType, Priority::create(10));
			dwarf1Id = actors.getId(dwarf1);
			// One step to find the plant needing water.
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
		REQUIRE(objective2.getTypeId() == ObjectiveType::getIdByName("give plants fluid"));
		GivePlantsFluidObjective& harvestObjective = static_cast<GivePlantsFluidObjective&>(objective2);
		REQUIRE(harvestObjective.getPlantLocation() == blocks2.getIndex_i(1,7,1));
	}
}