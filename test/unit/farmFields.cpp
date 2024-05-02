#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/plant.h"
#include "../../engine/farmFields.h"
#include "../../engine/cuboid.h"
#include "../../engine/threadedTask.h"
#include "../../engine/faction.h"
#include "../../engine/objectives/sowSeeds.h"
#include "../../engine/objectives/harvest.h"
#include "../../engine/objectives/givePlantsFluid.h"
#include "../../engine/objectives/goTo.h"
#include "actor.h"
#include "materialType.h"
#include <memory>
TEST_CASE("sow")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
	Simulation simulation(L"", DateTime(10, dayBeforeSowingStarts, 1200).toSteps());
	Area& area = simulation.createArea(10,10,10);
	Block& block = area.getBlock(4, 4, 2);
	Block& pondLocation = area.getBlock(8,8,2);
	pondLocation.setNotSolid();
	pondLocation.m_hasFluids.addFluid(Config::maxBlockVolume, FluidType::byName("water"));
	Block& foodLocation = area.getBlock(8,9,2);
	Item& food = simulation.createItemGeneric(ItemType::byName("apple"), MaterialType::byName("plant matter"), 50);
	food.setLocation(foodLocation);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(block, block);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	Actor& actor = simulation.createActor(ActorParamaters{
		.species=dwarf, 
		.location=&area.getBlock(1, 1, 2),
		.hasCloths=false,
		.hasSidearm=false,
	});
	actor.setFaction(&faction);
	SUBCASE("success")
	{
		REQUIRE(field.plantSpecies == nullptr);
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		REQUIRE(field.plantSpecies == &wheatGrass);
		REQUIRE(block.m_isPartOfFarmField.contains(faction));
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		const SowSeedsObjectiveType objectiveType;
		// Skip ahead to planting time.
		simulation.fastForward(Config::stepsPerDay);
		REQUIRE(objectiveType.canBeAssigned(actor));
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		REQUIRE(!block.m_reservable.isFullyReserved(&faction));
		REQUIRE(actor.m_hasObjectives.hasCurrent());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sow seeds");
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		simulation.doStep();
		REQUIRE(block.m_reservable.isFullyReserved(&faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		Block* destination = actor.m_canMove.getDestination();
		simulation.fastForwardUntillActorIsAtDestination(actor, *destination);
		REQUIRE(actor.m_canMove.getPath().empty());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		simulation.fastForward(Config::sowSeedsStepsDuration);
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!area.m_hasPlants.getAll().empty());
		REQUIRE(!objectiveType.canBeAssigned(actor));
		Plant& plant = area.m_hasPlants.getAll().front();
		REQUIRE(plant.m_percentGrown == 0);
		REQUIRE(&plant.m_plantSpecies == &wheatGrass);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "sow seeds");
		plant.die();
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		area.m_hasFarmFields.at(faction).remove(field);
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer accessable to sow")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 3, 2), area.getBlock(8, 3, 2), marble);
		Block& gateway = area.getBlock(9, 3, 2);
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		gateway.setSolid(marble);
		simulation.fastForwardUntillActorIsAdjacentTo(actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "sow seeds");
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer viable to sow")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
		block.getBlockBelow()->setSolid(marble);
		REQUIRE(!block.m_hasPlant.canGrowHereAtSomePointToday(wheatGrass));
		simulation.fastForwardUntillActorIsAdjacentTo(actor, block);
		// No alternative route or location found, cannot complete objective.
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "sow seeds");
	}
	SUBCASE("location no longer selected to sow")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
		area.m_hasFarmFields.at(faction).remove(field);
		simulation.fastForwardUntillActorIsAdjacentTo(actor, block);
		// Block is not select to grow anymore, cannot complete task, search for another route / another location to sow.
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "sow seeds");
	}
	SUBCASE("player cancels sowing objective")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		actor.m_hasObjectives.cancel(actor.m_hasObjectives.getCurrent());
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		REQUIRE(!block.m_reservable.isFullyReserved(&faction));
	}
	SUBCASE("player delays sowing objective")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(actor, block);
		actor.m_hasObjectives.addTaskToStart(std::move(goToObjective));
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		REQUIRE(!block.m_reservable.isFullyReserved(&faction));
	}
}
TEST_CASE("harvest")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	uint16_t dayBeforeHarvest = wheatGrass.harvestData->dayOfYearToStart - 1u;
	Simulation simulation(L"", DateTime(1, dayBeforeHarvest, 1200).toSteps());
	Area& area = simulation.createArea(10,10,10);
	Block& block = area.getBlock(4, 4, 2);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(block, block);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
	actor.setFaction(&faction);
	SUBCASE("harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass, 100);
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(block.m_hasPlant.get().m_quantityToHarvest == wheatGrass.harvestData->itemQuantity);
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		REQUIRE(actor.m_hasObjectives.hasCurrent());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "harvest");
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		simulation.doStep();
		REQUIRE(block.m_reservable.isFullyReserved(&faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		Block* destination = actor.m_canMove.getDestination();
		simulation.fastForwardUntillActorIsAtDestination(actor, *destination);
		REQUIRE(actor.m_canMove.getPath().empty());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
		simulation.fastForward(Config::harvestEventDuration);
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!objectiveType.canBeAssigned(actor));
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		Item& item = **actor.m_location->m_hasItems.getAll().begin();
		REQUIRE(item.m_materialType == MaterialType::byName("plant matter"));
		REQUIRE(item.getQuantity() + block.m_hasPlant.get().m_quantityToHarvest == wheatGrass.harvestData->itemQuantity);
		REQUIRE(item.m_itemType == ItemType::byName("wheat seed"));
	}
	SUBCASE("location no longer accessable to harvest")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 3, 2), area.getBlock(8, 3, 2), marble);
		Block& gateway = area.getBlock(9, 3, 2);
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		gateway.setSolid(marble);
		simulation.fastForwardUntillActorIsAdjacentTo(actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "harvest");
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("location no longer contains plant")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		block.m_hasPlant.erase();
		simulation.fastForwardUntillActorIsAdjacentTo(actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("plant no longer harvestable")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		block.m_hasPlant.get().endOfHarvest();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		simulation.fastForwardUntillActorIsAdjacentTo(actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "harvest");
	}
	SUBCASE("player cancels harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
		actor.m_hasObjectives.cancel(actor.m_hasObjectives.getCurrent());
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
	}
	SUBCASE("player delays harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		actor.satisfyNeeds();
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(actor, block);
		actor.m_hasObjectives.addTaskToStart(std::move(goToObjective));
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
	}
	SUBCASE("give plants fluid")
	{
		static const FluidType& water = FluidType::byName("water");
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.createPlant(wheatGrass);
		Plant& plant = block.m_hasPlant.get();
		REQUIRE(!area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		REQUIRE(plant.m_growthEvent.exists());
		Item& bucket = simulation.createItemNongeneric(ItemType::byName("bucket"), MaterialType::byName("poplar wood"), 50u, 0);
		Block& bucketLocation = area.getBlock(7, 7, 2);
		bucket.setLocation(bucketLocation);
		Block& pondLocation = area.getBlock(3, 9, 1);
		pondLocation.setNotSolid();
		pondLocation.m_hasFluids.addFluid(100, water);
		REQUIRE(actor.m_canPickup.canPickupAny(bucket));
		plant.setMaybeNeedsFluid();
		REQUIRE(block.m_hasPlant.exists());
		REQUIRE(plant.m_volumeFluidRequested != 0);
		REQUIRE(!plant.m_growthEvent.exists());
		REQUIRE(area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		const GivePlantsFluidObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(actor));
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		simulation.doStep();
		actor.satisfyNeeds();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "give plants fluid");
		simulation.doStep();
		REQUIRE(bucket.isAdjacentTo(*actor.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToDestination(actor, bucketLocation);
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		REQUIRE(actor.m_canPickup.isCarryingEmptyContainerWhichCanHoldFluid());
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		REQUIRE(actor.m_canMove.getDestination() != nullptr);
		REQUIRE(actor.m_canMove.getDestination()->isAdjacentToIncludingCornersAndEdges(pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(actor, pondLocation);
		REQUIRE(bucket.m_hasCargo.containsAnyFluid());
		REQUIRE(bucket.m_hasCargo.getFluidType() == water);
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
		REQUIRE(actor.m_canMove.getPath().empty());
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		REQUIRE(!actor.m_canMove.getPath().empty());
		Block* destination = actor.m_canMove.getDestination();
		simulation.fastForwardUntillActorIsAtDestination(actor, *destination);
		simulation.fastForward(Config::givePlantsFluidDelaySteps);
		REQUIRE(plant.m_volumeFluidRequested == 0);
		REQUIRE(plant.m_growthEvent.exists());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "give plants fluid");
	}
}
