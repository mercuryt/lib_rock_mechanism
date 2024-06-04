#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include "../../engine/objectives/sowSeeds.h"
#include "../../engine/objectives/harvest.h"
#include "../../engine/objectives/givePlantsFluid.h"
#include "../../engine/objectives/goTo.h"
TEST_CASE("json")
{
	Simulation simulation{L"", DateTime(12, 50, 1000).toSteps()};
	Faction& faction = simulation.createFaction(L"tower of power");
	const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	const MaterialType& dirt = MaterialType::byName("dirt");
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& cotton = MaterialType::byName("cotton");
	const MaterialType& bronze = MaterialType::byName("bronze");
	const MaterialType& sand = MaterialType::byName("sand");
	const PlantSpecies& sage = PlantSpecies::byName("sage brush");
	const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	const FluidType& water = FluidType::byName("water");
	const ItemType& axe = ItemType::byName("axe");
	const ItemType& pick = ItemType::byName("pick");
	const ItemType& saw = ItemType::byName("saw");
	const ItemType& bucket = ItemType::byName("bucket");
	const ItemType& preparedMeal = ItemType::byName("prepared meal");
	const ItemType& pants = ItemType::byName("pants");
	const ItemType& pile = ItemType::byName("pile");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	area.m_blockDesignations.registerFaction(faction);
	areaBuilderUtil::setSolidLayer(area, 0, dirt);
	area.m_hasFarmFields.registerFaction(faction);

	SUBCASE("basic")
	{
		Cuboid farmBlocks{blocks, blocks.getIndex({1,7,1}), blocks.getIndex({1,6,1})};
		FarmField& farm = area.m_hasFarmFields.at(faction).create(farmBlocks);
		area.m_hasFarmFields.at(faction).setSpecies(farm, sage);
		Actor& dwarf1 = simulation.m_hasActors->createActor(ActorParamaters{
			.species=dwarf, 
			.percentGrown=90,
			.location=blocks.getIndex({5,5,1}),
			.faction=&faction,
			.hasCloths=false,
			.hasSidearm=false,
		});
		dwarf1.setFaction(&faction);
		std::wstring name = dwarf1.m_name;
		DigObjectiveType& digObjectiveType = static_cast<DigObjectiveType&>(*ObjectiveType::objectiveTypes.at("dig").get());
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, blocks.getIndex({9,9,1}));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 10);
		dwarf1.m_hasObjectives.replaceTasks(std::move(objective));
		blocks.plant_create(blocks.getIndex({8, 8, 1}), sage, 99);
		Plant& sage1 = blocks.plant_get(blocks.getIndex({8,8,1}));
		sage1.setMaybeNeedsFluid();
		blocks.fluid_add(blocks.getIndex({3, 8, 1}), 10, water);
		Item& axe1 = simulation.m_hasItems->createItemNongeneric(axe, bronze, 10, 10);
		axe1.setLocation(blocks.getIndex({1,2,1}));
		blocks.blockFeature_construct(blocks.getIndex({1,8,1}), BlockFeatureType::stairs, wood);
		blocks.blockFeature_construct(blocks.getIndex({9,1,1}), BlockFeatureType::door, wood);
		area.m_fires.ignite(blocks.getIndex({9,1,1}), wood);
		Item& pants1 = simulation.m_hasItems->createItemNongeneric(pants, cotton, 50, 30);
		dwarf1.m_equipmentSet.addEquipment(pants1);
		Item& saw1 = simulation.m_hasItems->createItemNongeneric(saw, bronze, 50, 30);
		dwarf1.m_canPickup.pickUp(saw1);
		Item& bucket1 = simulation.m_hasItems->createItemNongeneric(bucket, bronze, 50, 30);
		bucket1.setLocation(blocks.getIndex({0,0,1}));
		bucket1.m_hasCargo.add(water, 5);
		Item& bucket2 = simulation.m_hasItems->createItemNongeneric(bucket, bronze, 50, 30);
		bucket2.setLocation(blocks.getIndex({0,1,1}));
		bucket2.m_hasCargo.add(pile, sand, 1);

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		// BlockIndex.
		REQUIRE(blocks2.m_sizeX == blocks2.m_sizeY);
		REQUIRE(blocks2.m_sizeX == blocks2.m_sizeZ);
		REQUIRE(blocks2.m_sizeX == 10);
		REQUIRE(blocks2.solid_is(blocks2.getIndex({5,5,0})));
		REQUIRE(blocks2.solid_get(blocks2.getIndex({5,5,0})) == dirt);
		// Plant.
		REQUIRE(blocks2.plant_exists(blocks2.getIndex({8,8,1})));
		Plant& sage2 = blocks.plant_get(blocks2.getIndex({8,8,1}));
		REQUIRE(sage2.m_plantSpecies == sage);
		REQUIRE(sage2.getGrowthPercent() == 99);
		REQUIRE(sage2.getStepAtWhichPlantWillDieFromLackOfFluid());
		REQUIRE(sage2.m_fluidEvent.exists());
		REQUIRE(!sage2.m_growthEvent.exists());
		// Fluid.
		BlockIndex waterLocation = blocks2.getIndex({3,8,1});
		REQUIRE(blocks.fluid_getTotalVolume(waterLocation) == 10);
		REQUIRE(blocks.fluid_volumeOfTypeContains(waterLocation, water) == 10);
		FluidGroup& fluidGroup = *blocks.fluid_getGroup(waterLocation, water);
		REQUIRE(area2.m_hasFluidGroups.getUnstable().contains(&fluidGroup));
		// Actor.
		REQUIRE(!blocks2.actor_empty(blocks2.getIndex({5,5,1})));
		Actor* dwarf2 = blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];
		REQUIRE(&dwarf2->m_species == &dwarf);
		REQUIRE(dwarf2->m_canGrow.growthPercent() == 90);
		REQUIRE(dwarf2->m_name == name);
		REQUIRE(dwarf2->m_mustEat.hasHungerEvent());
		REQUIRE(dwarf2->m_mustDrink.thirstEventExists());
		REQUIRE(dwarf2->m_mustSleep.hasTiredEvent());
		REQUIRE(dwarf2->getFaction() == &faction2);
		// Objective.
		REQUIRE(dwarf2->m_hasObjectives.m_prioritySet.getPriorityFor(ObjectiveTypeId::Dig) == 10);
		REQUIRE(dwarf2->m_hasObjectives.getCurrent().getObjectiveTypeId() == ObjectiveTypeId::GoTo);
		GoToObjective& goToObjective = static_cast<GoToObjective&>(dwarf2->m_hasObjectives.getCurrent());
		REQUIRE(goToObjective.getLocation() == blocks2.getIndex({9,9,1}));
		// Equipment.
		REQUIRE(!dwarf2->m_equipmentSet.getAll().empty());
		Item* pants2 = *dwarf2->m_equipmentSet.getAll().begin();
		REQUIRE(pants2->m_itemType == pants);
		REQUIRE(pants2->m_materialType == cotton);
		// canPickup.
		REQUIRE(dwarf2->m_canPickup.isCarryingAnything());
		REQUIRE(dwarf2->m_canPickup.getCarrying()->isItem());
		Item& saw2 = dwarf2->m_canPickup.getItem();
		REQUIRE(saw2.m_itemType == saw);
		REQUIRE(saw2.m_materialType == bronze);
		// Items
		REQUIRE(!blocks2.item_empty(blocks2.getIndex({1,2,1})));
		Item* axe2 = blocks2.item_getAll(blocks2.getIndex({1,2,1}))[0];
		REQUIRE(axe2->m_materialType == bronze);
		REQUIRE(axe2->m_quality == 10);
		REQUIRE(axe2->m_percentWear == 10);
		// BlockIndex features.
		REQUIRE(blocks2.blockFeature_contains(blocks2.getIndex({1,8,1}), BlockFeatureType::stairs));
		REQUIRE(blocks2.blockFeature_atConst(blocks2.getIndex({1,8,1}), BlockFeatureType::stairs)->materialType == &wood);
		REQUIRE(blocks2.blockFeature_contains(blocks2.getIndex({9,1,1}), BlockFeatureType::door));
		// Item cargo.
		REQUIRE(!blocks2.item_empty(blocks2.getIndex({0,0,1})));
		REQUIRE(blocks2.item_getAll(blocks2.getIndex({0,0,1})).size() == 1);
		Item* bucket3 = blocks2.item_getAll(blocks2.getIndex({0,0,1}))[0];
		REQUIRE(bucket3);
		REQUIRE(bucket3->m_hasCargo.containsAnyFluid());
		REQUIRE(bucket3->m_hasCargo.getFluidType() == water);
		REQUIRE(bucket3->m_hasCargo.getFluidVolume() == 5);
		REQUIRE(blocks2.item_getAll(blocks2.getIndex({0,1,1})).size() == 1);
		Item* bucket4 = blocks2.item_getAll(blocks2.getIndex({0,1,1}))[0];
		REQUIRE(bucket4);
		REQUIRE(bucket4->m_hasCargo.containsGeneric(pile, sand, 1));
		// Fires.
		REQUIRE(area2.m_fires.contains(blocks2.getIndex({9,1,1}), wood));
		Fire& fire = area2.m_fires.at(blocks2.getIndex({9,1,1}), wood);
		REQUIRE(fire.m_stage == FireStage::Smouldering);
		REQUIRE(fire.m_hasPeaked == false);
		REQUIRE(fire.m_event.exists());
		// Farm fields.
		REQUIRE(area2.m_hasFarmFields.contains(faction2));
		REQUIRE(blocks2.farm_contains(blocks2.getIndex({1,6,1}), faction2));
		REQUIRE(blocks2.farm_get(blocks2.getIndex({1,6,1}), faction2)->plantSpecies == &sage);
		REQUIRE(blocks2.farm_contains(blocks2.getIndex({1,7,1}), faction2));
		// OpacityFacade.
		area2.m_hasActors.m_opacityFacade.validate();
	}
	SUBCASE("dig project")
	{
		BlockIndex holeLocation = blocks.getIndex({8, 4, 0});
		area.m_hasDigDesignations.addFaction(faction);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		Item& pick1 = simulation.m_hasItems->createItemNongeneric(pick, bronze, 10, 10);
		pick1.setLocation(blocks.getIndex({1,2,1}));
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		DigObjectiveType& digObjectiveType = static_cast<DigObjectiveType&>(*ObjectiveType::objectiveTypes.at("dig").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		BlockIndex holeLocation2 = blocks2.getIndex({8,4,0});
		REQUIRE(area2.m_hasDigDesignations.contains(faction2, holeLocation2));
		DigProject& project = area2.m_hasDigDesignations.at(faction2, holeLocation2);
		REQUIRE(project.getWorkersAndCandidates().size() == 1);
		REQUIRE(project.getWorkers().size() == 1);
		Actor& dwarf2 = *blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];
		REQUIRE(dwarf2.m_project == static_cast<Project*>(&project));
		REQUIRE(!dwarf2.m_canMove.getPath().empty());
		DigObjective& objective = static_cast<DigObjective&>(dwarf2.m_hasObjectives.getCurrent());
		REQUIRE(objective.getProject()  == dwarf2.m_project);
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf2);
		REQUIRE(projectWorker.haulSubproject);
		REQUIRE(projectWorker.objective == dwarf2.m_hasObjectives.getCurrent());
		HaulSubproject& haulSubproject = *projectWorker.haulSubproject;
		REQUIRE(haulSubproject.getWorkers().contains(&dwarf2));
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Individual);
		Item* pick2 = blocks2.item_getAll(blocks2.getIndex({1,2,1}))[0];
		REQUIRE(haulSubproject.getToHaul().isItem());
		REQUIRE(static_cast<Item*>(&haulSubproject.getToHaul()) == pick2);
		REQUIRE(haulSubproject.getQuantity() == 1);
		REQUIRE(!haulSubproject.getIsMoving());
		REQUIRE(!project.hasTryToHaulEvent());
		REQUIRE(!project.hasTryToHaulThreadedTask());
		REQUIRE(!project.hasTryToReserveEvent());
		REQUIRE(!project.hasTryToAddWorkersThreadedTask());
	}
	SUBCASE("construct project")
	{
		BlockIndex projectLocation = blocks.getIndex({8, 4, 1});
		area.m_hasConstructionDesignations.addFaction(faction);
		area.m_hasConstructionDesignations.designate(faction, projectLocation, nullptr, wood);
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		ConstructObjectiveType& constructObjectiveType = static_cast<ConstructObjectiveType&>(*ObjectiveType::objectiveTypes.at("construct").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		BlockIndex projectLocation2 = blocks2.getIndex({8,4,1});
		REQUIRE(area2.m_hasConstructionDesignations.contains(faction2, projectLocation2));
		ConstructProject& project = area2.m_hasConstructionDesignations.getProject(faction2, projectLocation2);
		REQUIRE(project.getMaterialType() == wood);
		// The project is not yet active and the actor is a candidate rather then a worker.
		REQUIRE(project.getWorkersAndCandidates().size() == 1);
		REQUIRE(project.getWorkers().size() == 0);
		Actor& dwarf2 = *blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];
		REQUIRE(dwarf2.m_project == static_cast<Project*>(&project));
		REQUIRE(dwarf2.m_canMove.getPath().empty());
		ConstructObjective& objective = static_cast<ConstructObjective&>(dwarf2.m_hasObjectives.getCurrent());
		REQUIRE(objective.getProject() == dwarf2.m_project);
		REQUIRE(!project.hasTryToHaulEvent());
		REQUIRE(!project.hasTryToHaulThreadedTask());
		REQUIRE(!project.hasTryToReserveEvent());
		REQUIRE(project.hasTryToAddWorkersThreadedTask());
	}
	SUBCASE("stockpile project")
	{
		BlockIndex projectLocation1 = blocks.getIndex({8, 4, 1});
		BlockIndex pileLocation1 = blocks.getIndex({1, 4, 1});
		area.m_hasStockPiles.registerFaction(faction);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile);
		StockPile& stockPile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockPile.addBlock(projectLocation1);
		Item& pile1 = simulation.m_hasItems->createItemGeneric(pile, dirt, 10);
		pile1.setLocation(pileLocation1);
		REQUIRE(stockPile.accepts(pile1));
		area.m_hasStockPiles.at(faction).addItem(pile1);
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		StockPileObjectiveType& stockPileObjectiveType = static_cast<StockPileObjectiveType&>(*ObjectiveType::objectiveTypes.at("stockpile").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(stockPileObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		BlockIndex projectLocation2 = blocks2.getIndex({8,4,1});
		BlockIndex pileLocation2 = blocks2.getIndex({1, 4, 1});
		Item& pile2 = *blocks2.item_getAll(pileLocation2)[0];
		Actor& dwarf2 = *blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];

		REQUIRE(blocks2.stockpile_contains(projectLocation2, faction2));
		StockPile& stockPile2 = *blocks2.stockpile_getForFaction(projectLocation2, faction2);
		REQUIRE(stockPile2.accepts(pile2));
		REQUIRE(area2.m_hasStockPiles.at(faction2).getItemsWithProjectsCount() == 1);
		StockPileObjective& objective = static_cast<StockPileObjective&>(dwarf2.m_hasObjectives.getCurrent());
		REQUIRE(objective.m_project);
		REQUIRE(dwarf2.m_project);
		StockPileProject& project = *objective.m_project;
		// The project is not yet active and the actor is a candidate rather then a worker.
		REQUIRE(project.getWorkersAndCandidates().size() == 1);
		REQUIRE(project.getWorkers().size() == 0);
		REQUIRE(dwarf2.m_project == static_cast<Project*>(&project));
		REQUIRE(dwarf2.m_canMove.getPath().empty());
		REQUIRE(project.getItem() == pile2);
		REQUIRE(project.getLocation() == projectLocation2);
	}
	SUBCASE("craft")
	{
		CraftStepTypeCategory& craftStepTypeSaw = CraftStepTypeCategory::byName("saw");
		CraftStepTypeCategory& craftStepTypeScrape = CraftStepTypeCategory::byName("scrape");
		area.m_hasCraftingLocationsAndJobs.addFaction(faction);
		area.m_hasCraftingLocationsAndJobs.at(faction).addLocation(craftStepTypeSaw, blocks.getIndex({3,3,1}));
		area.m_hasCraftingLocationsAndJobs.at(faction).addLocation(craftStepTypeScrape, blocks.getIndex({3,2,1}));
		CraftJobType& jobTypeSawBoards = CraftJobType::byName("saw boards");
		area.m_hasCraftingLocationsAndJobs.at(faction).addJob(jobTypeSawBoards, &wood, 1);
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		CraftObjectiveType& woodWorkingObjectiveType = static_cast<CraftObjectiveType&>(*ObjectiveType::objectiveTypes.at("wood working").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(woodWorkingObjectiveType, 100);
		Item& saw = simulation.m_hasItems->createItemNongeneric(ItemType::byName("saw"), bronze, 25, 0);
		saw.setLocation(blocks.getIndex({3, 7, 1}));
		Item& chisel = simulation.m_hasItems->createItemNongeneric(ItemType::byName("chisel"), bronze, 25, 0);
		chisel.setLocation(blocks.getIndex({3, 6, 1}));
		Item& log = simulation.m_hasItems->createItemGeneric(ItemType::byName("log"), wood, 1);
		log.setLocation(blocks.getIndex({3, 8, 1}));
		// One step to find the designation.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		Actor& dwarf2 = *blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];

		REQUIRE(area2.m_hasCraftingLocationsAndJobs.at(faction2).hasLocationsForCategory(craftStepTypeSaw));
		REQUIRE(area2.m_hasCraftingLocationsAndJobs.at(faction2).hasLocationsForCategory(craftStepTypeScrape));
		REQUIRE(area2.m_hasCraftingLocationsAndJobs.at(faction2).hasJobs());
		REQUIRE(dwarf2.m_project);
		REQUIRE(static_cast<CraftObjective&>(dwarf2.m_hasObjectives.getCurrent()).getCraftJob());
		CraftJob& job = *static_cast<CraftObjective&>(dwarf2.m_hasObjectives.getCurrent()).getCraftJob();
		REQUIRE(job.craftStepProject);
		REQUIRE(&job.craftJobType == &jobTypeSawBoards);
		REQUIRE(job.materialType == &wood);
		CraftStepProject& project = *job.craftStepProject.get();
		REQUIRE(static_cast<CraftStepProject*>(dwarf2.m_project) == &project);
	}
	SUBCASE("drink")
	{
		Item& bucket1 = simulation.m_hasItems->createItemNongeneric(bucket, bronze, 25, 0);
		bucket1.setLocation(blocks.getIndex({3, 7, 1}));
		bucket1.m_hasCargo.add(water, 10);
		simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		// One step to find the bucket.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actor& dwarf2 = *blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];

		REQUIRE(dwarf2.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		REQUIRE(blocks2.isAdjacentToIncludingCornersAndEdges(dwarf2.m_canMove.getDestination(), blocks2.getIndex({3,7,1})));
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().getObjectiveTypeId() == ObjectiveTypeId::Drink);
	}
	SUBCASE("eat")
	{
		Item& preparedMeal1 = simulation.m_hasItems->createItemNongeneric(preparedMeal, MaterialType::byName("fruit"), 25, 0);
		preparedMeal1.setLocation(blocks.getIndex({3, 7, 1}));
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		simulation.fastForward(dwarf.stepsEatFrequency);
		// Discard drink objective if exists.
		if(dwarf1.m_mustDrink.getVolumeFluidRequested() != 0)
			dwarf1.m_mustDrink.drink(dwarf1.m_mustDrink.getVolumeFluidRequested());
		// One step to find the meal.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actor& dwarf2 = *blocks2.actor_getAll(blocks2.getIndex({5,5,1}))[0];

		REQUIRE(dwarf2.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		REQUIRE(blocks2.isAdjacentToIncludingCornersAndEdges(dwarf2.m_canMove.getDestination(), blocks2.getIndex({3,7,1})));
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().getObjectiveTypeId() == ObjectiveTypeId::Eat);
	}
	SUBCASE("sleep")
	{
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		area.m_hasSleepingSpots.designate(faction, blocks.getIndex({2,2,1}));
		area.m_hasSleepingSpots.designate(faction, blocks.getIndex({1,1,1}));
		simulation.fastForward(dwarf.stepsSleepFrequency);
		// Discard drink objective if exists.
		if(dwarf1.m_mustDrink.getVolumeFluidRequested() != 0)
			dwarf1.m_mustDrink.drink(dwarf1.m_mustDrink.getVolumeFluidRequested());
		if(dwarf1.m_mustEat.getMassFoodRequested() != 0)
			dwarf1.m_mustEat.eat(dwarf1.m_mustEat.getMassFoodRequested());
		ActorId dwarf1Id = dwarf1.m_id;

		// One step to find the sleeping spot.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actor& dwarf2 = simulation2.m_hasActors->getById(dwarf1Id);

		REQUIRE(dwarf2.m_mustSleep.hasTiredEvent());
		REQUIRE(dwarf2.m_mustSleep.getSleepPercent() == 0);
		REQUIRE(area2.m_hasSleepingSpots.containsUnassigned(blocks2.getIndex({1,1,1})));
		REQUIRE(dwarf2.m_canMove.getDestination());
		REQUIRE(blocks2.isAdjacentToIncludingCornersAndEdges(dwarf2.m_canMove.getDestination(), blocks2.getIndex({2,2,1})));
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().getObjectiveTypeId() == ObjectiveTypeId::Sleep);
		REQUIRE(dwarf2.m_mustSleep.getLocation() == BLOCK_INDEX_MAX);
	}
	SUBCASE("sow seed")
	{

		Cuboid farmBlocks{blocks, blocks.getIndex({1,7,1}), blocks.getIndex({1,6,1})};
		FarmField& farm = area.m_hasFarmFields.at(faction).create(farmBlocks);
		area.m_hasFarmFields.at(faction).setSpecies(farm, sage);
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		SowSeedsObjectiveType& sowObjectiveType = static_cast<SowSeedsObjectiveType&>(*ObjectiveType::objectiveTypes.at("sow seeds").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(sowObjectiveType, 10);
		ActorId dwarf1Id = dwarf1.m_id;
		
		// One step to find the sowing location.
		simulation.doStep();

		SowSeedsObjective& objective = static_cast<SowSeedsObjective&>(dwarf1.m_hasObjectives.getCurrent());
		BlockIndex block1 = objective.getBlock();
		Point3D coordinates1 = area.getBlocks().getCoordinates(block1);

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		[[maybe_unused]] Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actor& dwarf2 = simulation2.m_hasActors->getById(dwarf1Id);
		
		Objective& objective2 = dwarf2.m_hasObjectives.getCurrent();
		REQUIRE(objective2.getObjectiveTypeId() == ObjectiveTypeId::SowSeeds);
		REQUIRE(dwarf2.m_canMove.getDestination());
		BlockIndex block2 = objective.getBlock();
		Point3D coordinates2 = blocks2.getCoordinates(block2);
		REQUIRE(coordinates2.x == coordinates1.x);
		REQUIRE(coordinates2.y == coordinates1.y);
		REQUIRE(coordinates2.z == coordinates1.z);
	}
	SUBCASE("harvest")
	{
		blocks.plant_create(blocks.getIndex({1,7,1}), wheatGrass, 100);
		Plant& plant = blocks.plant_get(blocks.getIndex({1,7,1}));
		plant.setQuantityToHarvest();
		REQUIRE(plant.m_quantityToHarvest);
		Cuboid farmBlocks{blocks, blocks.getIndex({1,7,1}), blocks.getIndex({1,6,1})};
		FarmField& farm = area.m_hasFarmFields.at(faction).create(farmBlocks);
		area.m_hasFarmFields.at(faction).setSpecies(farm, wheatGrass);
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		HarvestObjectiveType& harvestObjectiveType = static_cast<HarvestObjectiveType&>(*ObjectiveType::objectiveTypes.at("harvest").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(harvestObjectiveType, 10);
		ActorId dwarf1Id = dwarf1.m_id;
		
		// One step to find the harvest location.
		simulation.doStep();
		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		[[maybe_unused]] Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actor& dwarf2 = simulation2.m_hasActors->getById(dwarf1Id);
		
		Objective& objective2 = dwarf2.m_hasObjectives.getCurrent();
		REQUIRE(objective2.getObjectiveTypeId() == ObjectiveTypeId::Harvest);
		HarvestObjective& harvestObjective = static_cast<HarvestObjective&>(objective2);
		REQUIRE(harvestObjective.getBlock() == blocks2.getIndex({1,7,1}));
	}
	SUBCASE("give fluid to plant")
	{
		Item& bucket1 = simulation.m_hasItems->createItemNongeneric(bucket, bronze, 25, 0);
		bucket1.setLocation(blocks.getIndex({3, 7, 1}));
		bucket1.m_hasCargo.add(water, 10);
		blocks.plant_create(blocks.getIndex({1,7,1}), wheatGrass, 100);
		Plant& plant = blocks.plant_get(blocks.getIndex({1,7,1}));
		plant.setMaybeNeedsFluid();
		REQUIRE(plant.m_volumeFluidRequested);
		Cuboid farmBlocks{blocks, blocks.getIndex({1,7,1}), blocks.getIndex({1,6,1})};
		FarmField& farm = area.m_hasFarmFields.at(faction).create(farmBlocks);
		area.m_hasFarmFields.at(faction).setSpecies(farm, wheatGrass);
		Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, blocks.getIndex({5,5,1}), 90);
		dwarf1.setFaction(&faction);
		GivePlantsFluidObjectiveType& harvestObjectiveType = static_cast<GivePlantsFluidObjectiveType&>(*ObjectiveType::objectiveTypes.at("give plants fluid").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(harvestObjectiveType, 10);
		ActorId dwarf1Id = dwarf1.m_id;
		
		// One step to find the plant needing water.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		[[maybe_unused]] Area& area2 = simulation2.m_hasAreas->loadAreaFromJson(areaData, simulation2.getDeserializationMemo());
		Blocks& blocks2 = area2.getBlocks();
		Actor& dwarf2 = simulation2.m_hasActors->getById(dwarf1Id);
		
		Objective& objective2 = dwarf2.m_hasObjectives.getCurrent();
		REQUIRE(objective2.getObjectiveTypeId() == ObjectiveTypeId::GivePlantsFluid);
		GivePlantsFluidObjective& harvestObjective = static_cast<GivePlantsFluidObjective&>(objective2);
		REQUIRE(harvestObjective.getPlantLocation() == blocks2.getIndex({1,7,1}));
	}
}
