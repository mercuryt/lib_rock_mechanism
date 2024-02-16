#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include "../../engine/goTo.h"
TEST_CASE("json")
{
	Simulation simulation;
	Faction& faction = simulation.createFaction(L"tower of power");
	const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	const MaterialType& dirt = MaterialType::byName("dirt");
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& cotton = MaterialType::byName("cotton");
	const MaterialType& bronze = MaterialType::byName("bronze");
	const MaterialType& sand = MaterialType::byName("sand");
	const PlantSpecies& sage = PlantSpecies::byName("sage brush");
	const FluidType& water = FluidType::byName("water");
	const ItemType& axe = ItemType::byName("axe");
	const ItemType& pick = ItemType::byName("pick");
	const ItemType& saw = ItemType::byName("saw");
	const ItemType& bucket = ItemType::byName("bucket");
	const ItemType& pants = ItemType::byName("pants");
	const ItemType& pile = ItemType::byName("pile");
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, dirt);
	area.m_hasFarmFields.registerFaction(faction);

	SUBCASE("basic")
	{
		Cuboid farmBlocks{area.getBlock(1,7,1), area.getBlock(1,6,1)};
		FarmField& farm = area.m_hasFarmFields.at(faction).create(farmBlocks);
		area.m_hasFarmFields.at(faction).setSpecies(farm, sage);
		Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(5,5,1), 90);
		dwarf1.setFaction(&faction);
		std::wstring name = dwarf1.m_name;
		DigObjectiveType& digObjectiveType = static_cast<DigObjectiveType&>(*ObjectiveType::objectiveTypes.at("dig").get());
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, area.getBlock(9,9,1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 10);
		dwarf1.m_hasObjectives.replaceTasks(std::move(objective));
		area.getBlock(8, 8, 1).m_hasPlant.createPlant(sage, 99);
		Plant& sage1 = area.getBlock(8,8,1).m_hasPlant.get();
		sage1.setMaybeNeedsFluid();
		area.getBlock(3, 8, 1).addFluid(10, water);
		Item& axe1 = simulation.createItemNongeneric(axe, bronze, 10, 10);
		axe1.setLocation(area.getBlock(1,2,1));
		area.getBlock(1,8,1).m_hasBlockFeatures.construct(BlockFeatureType::stairs, wood);
		area.getBlock(9,1,1).m_hasBlockFeatures.construct(BlockFeatureType::door, wood);
		area.m_fires.ignite(area.getBlock(9,1,1), wood);
		Item& pants1 = simulation.createItemNongeneric(pants, cotton, 50, 30);
		dwarf1.m_equipmentSet.addEquipment(pants1);
		Item& saw1 = simulation.createItemNongeneric(saw, bronze, 50, 30);
		dwarf1.m_canPickup.pickUp(saw1);
		Item& bucket1 = simulation.createItemNongeneric(bucket, bronze, 50, 30);
		bucket1.setLocation(area.getBlock(0,0,1));
		bucket1.m_hasCargo.add(water, 5);
		Item& bucket2 = simulation.createItemNongeneric(bucket, bronze, 50, 30);
		bucket2.setLocation(area.getBlock(0,1,1));
		bucket2.m_hasCargo.add(pile, sand, 1);

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.loadAreaFromJson(areaData);
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		// Block.
		REQUIRE(area2.m_sizeX == area2.m_sizeY);
		REQUIRE(area2.m_sizeX == area2.m_sizeZ);
		REQUIRE(area2.m_sizeX == 10);
		REQUIRE(area2.getBlock(5,5,0).isSolid());
		REQUIRE(area2.getBlock(5,5,0).getSolidMaterial() == dirt);
		// Plant.
		REQUIRE(area2.getBlock(8,8,1).m_hasPlant.exists());
		Plant& sage2 = area2.getBlock(8,8,1).m_hasPlant.get();
		REQUIRE(sage2.m_plantSpecies == sage);
		REQUIRE(sage2.getGrowthPercent() == 99);
		REQUIRE(sage2.getStepAtWhichPlantWillDieFromLackOfFluid());
		REQUIRE(sage2.m_fluidEvent.exists());
		REQUIRE(!sage2.m_growthEvent.exists());
		// Fluid.
		Block& waterLocation = area2.getBlock(3,8,1);
		REQUIRE(waterLocation.m_totalFluidVolume == 10);
		REQUIRE(waterLocation.volumeOfFluidTypeContains(water) == 10);
		FluidGroup& fluidGroup = *waterLocation.getFluidGroup(water);
		REQUIRE(area2.m_unstableFluidGroups.contains(&fluidGroup));
		// Actor.
		REQUIRE(!area2.getBlock(5,5,1).m_hasActors.empty());
		Actor* dwarf2 = area2.getBlock(5,5,1).m_hasActors.getAll()[0];
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
		REQUIRE(goToObjective.getLocation() == area2.getBlock(9,9,1));
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
		REQUIRE(!area2.getBlock(1,2,1).m_hasItems.empty());
		Item* axe2 = area2.getBlock(1,2,1).m_hasItems.getAll()[0];
		REQUIRE(axe2->m_materialType == bronze);
		REQUIRE(axe2->m_quality == 10);
		REQUIRE(axe2->m_percentWear == 10);
		// Block features.
		REQUIRE(area2.getBlock(1,8,1).m_hasBlockFeatures.contains(BlockFeatureType::stairs));
		REQUIRE(area2.getBlock(1,8,1).m_hasBlockFeatures.at(BlockFeatureType::stairs)->materialType == &wood);
		REQUIRE(area2.getBlock(9,1,1).m_hasBlockFeatures.contains(BlockFeatureType::door));
		// Item cargo.
		REQUIRE(!area2.getBlock(0,0,1).m_hasItems.empty());
		Item* bucket3 = area2.getBlock(0,0,1).m_hasItems.getAll()[0];
		REQUIRE(bucket3->m_hasCargo.containsAnyFluid());
		REQUIRE(bucket3->m_hasCargo.getFluidType() == water);
		REQUIRE(bucket3->m_hasCargo.getFluidVolume() == 5);
		Item* bucket4 = area2.getBlock(0,1,1).m_hasItems.getAll()[0];
		REQUIRE(bucket4->m_hasCargo.containsGeneric(pile, sand, 1));
		// Fires.
		REQUIRE(area2.m_fires.contains(area2.getBlock(9,1,1), wood));
		Fire& fire = area2.m_fires.at(area2.getBlock(9,1,1), wood);
		REQUIRE(fire.m_stage == FireStage::Smouldering);
		REQUIRE(fire.m_hasPeaked == false);
		REQUIRE(fire.m_event.exists());
		// Farm fields.
		REQUIRE(area2.m_hasFarmFields.contains(faction2));
		REQUIRE(area2.getBlock(1,6,1).m_isPartOfFarmField.contains(faction2));
		REQUIRE(area2.getBlock(1,6,1).m_isPartOfFarmField.get(faction2)->plantSpecies == &sage);
		REQUIRE(area2.getBlock(1,7,1).m_isPartOfFarmField.contains(faction2));
	}
	SUBCASE("dig project")
	{
		Block& holeLocation = area.getBlock(8, 4, 0);
		area.m_hasDigDesignations.addFaction(faction);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		Item& pick1 = simulation.createItemNongeneric(pick, bronze, 10, 10);
		pick1.setLocation(area.getBlock(1,2,1));
		Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(5,5,1), 90);
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
		Area& area2 = simulation2.loadAreaFromJson(areaData);
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		Block& holeLocation2 = area2.getBlock(8,4,0);
		REQUIRE(area2.m_hasDigDesignations.contains(faction2, holeLocation2));
		DigProject& project = area2.m_hasDigDesignations.at(faction2, holeLocation2);
		REQUIRE(project.getWorkersAndCandidates().size() == 1);
		REQUIRE(project.getWorkers().size() == 1);
		Actor& dwarf2 = *area2.getBlock(5,5,1).m_hasActors.getAll()[0];
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
		Item* pick2 = area2.getBlock(1,2,1).m_hasItems.getAll()[0];
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
		Block& projectLocation = area.getBlock(8, 4, 1);
		area.m_hasConstructionDesignations.addFaction(faction);
		area.m_hasConstructionDesignations.designate(faction, projectLocation, nullptr, wood);
		Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(5,5,1), 90);
		dwarf1.setFaction(&faction);
		ConstructObjectiveType& constructObjectiveType = static_cast<ConstructObjectiveType&>(*ObjectiveType::objectiveTypes.at("construct").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.loadAreaFromJson(areaData);
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		Block& projectLocation2 = area2.getBlock(8,4,1);

		REQUIRE(area2.m_hasConstructionDesignations.contains(faction2, projectLocation2));
		ConstructProject& project = area2.m_hasConstructionDesignations.getProject(faction2, projectLocation2);
		REQUIRE(project.getMaterialType() == wood);
		// The project is not yet active and the actor is a candidate rather then a worker.
		REQUIRE(project.getWorkersAndCandidates().size() == 1);
		REQUIRE(project.getWorkers().size() == 0);
		Actor& dwarf2 = *area2.getBlock(5,5,1).m_hasActors.getAll()[0];
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
		Block& projectLocation1 = area.getBlock(8, 4, 1);
		Block& pileLocation1 = area.getBlock(1, 4, 1);
		area.m_hasStockPiles.addFaction(faction);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile);
		StockPile& stockPile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockPile.addBlock(projectLocation1);
		Item& pile1 = simulation.createItemGeneric(pile, dirt, 10);
		pile1.setLocation(pileLocation1);
		REQUIRE(stockPile.accepts(pile1));
		area.m_hasStockPiles.at(faction).addItem(pile1);
		Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(5,5,1), 90);
		dwarf1.setFaction(&faction);
		StockPileObjectiveType& stockPileObjectiveType = static_cast<StockPileObjectiveType&>(*ObjectiveType::objectiveTypes.at("stockpile").get());
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(stockPileObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();

		Json areaData = area.toJson();
		Json simulationData = simulation.toJson();
		Simulation simulation2(simulationData);
		Area& area2 = simulation2.loadAreaFromJson(areaData);
		Faction& faction2 = simulation2.m_hasFactions.byName(L"tower of power");
		Block& projectLocation2 = area2.getBlock(8,4,1);
		Block& pileLocation2 = area2.getBlock(1, 4, 1);
		Item& pile2 = *pileLocation2.m_hasItems.getAll()[0];
		Actor& dwarf2 = *area2.getBlock(5,5,1).m_hasActors.getAll()[0];

		REQUIRE(projectLocation2.m_isPartOfStockPiles.contains(faction2));
		StockPile& stockPile2 = *projectLocation2.m_isPartOfStockPiles.getForFaction(faction2);
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
}
