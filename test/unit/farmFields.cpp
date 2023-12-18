#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/animalSpecies.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/plant.h"
#include "../../src/farmFields.h"
#include "../../src/cuboid.h"
#include "../../src/faction.h"
#include "../../src/sowSeeds.h"
#include "../../src/threadedTask.h"
#include "../../src/harvest.h"
#include "../../src/givePlantsFluid.h"
#include "../../src/goTo.h"
#include "materialType.h"
#include "station.h"
#include <memory>
TEST_CASE("farmFields")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	Block& block = area.getBlock(4, 4, 2);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(block, block);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	Actor& actor = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
	actor.setFaction(&faction);
	SUBCASE("sow")
	{
		uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
		simulation.setDateTime({1, dayBeforeSowingStarts, 1200 });
		REQUIRE(field.plantSpecies == nullptr);
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		REQUIRE(field.plantSpecies == &wheatGrass);
		REQUIRE(block.m_isPartOfFarmField.contains(faction));
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		// Skip ahead to planting time.
		simulation.setDateTime({1, wheatGrass.dayOfYearForSowStart, 1200 });
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		REQUIRE(!block.m_reservable.isFullyReserved(&faction));
		const SowSeedsObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(actor));
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
		simulation.setDateTime({1, dayBeforeSowingStarts, 1200 });
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.setDateTime({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
		simulation.setDateTime({1, dayBeforeSowingStarts, 1200 });
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.setDateTime({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
		simulation.setDateTime({1, dayBeforeSowingStarts, 1200 });
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.setDateTime({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
		simulation.setDateTime({1, dayBeforeSowingStarts, 1200 });
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.setDateTime({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		simulation.doStep();
		REQUIRE(!actor.m_canMove.getPath().empty());
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		actor.m_hasObjectives.cancel(actor.m_hasObjectives.getCurrent());
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		REQUIRE(!block.m_reservable.isFullyReserved(&faction));
	}
	SUBCASE("player delays sowing objective")
	{
		uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
		simulation.setDateTime({1, dayBeforeSowingStarts, 1200 });
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.setDateTime({1, wheatGrass.dayOfYearForSowStart, 1200 });
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
	SUBCASE("harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.addPlant(wheatGrass, 100);
		uint16_t dayBeforeHarvest = wheatGrass.harvestData->dayOfYearToStart - 1u;
		simulation.setDateTime({1, dayBeforeHarvest, 1200});
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		// Skip ahead to harvest time.
		simulation.setDateTime({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		REQUIRE(item.getQuantity() == wheatGrass.harvestData->itemQuantity);
		REQUIRE(item.m_itemType == ItemType::byName("wheat seed"));
	}
	SUBCASE("location no longer accessable to harvest")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 3, 2), area.getBlock(8, 3, 2), marble);
		Block& gateway = area.getBlock(9, 3, 2);
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.addPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.setDateTime({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		block.m_hasPlant.addPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.setDateTime({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		block.m_hasPlant.addPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.setDateTime({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		block.m_hasPlant.addPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.setDateTime({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
		actor.m_hasObjectives.cancel(actor.m_hasObjectives.getCurrent());
		REQUIRE(block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
	}
	SUBCASE("player delays harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.addPlant(wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.setDateTime({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
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
		block.m_hasPlant.addPlant(wheatGrass);
		Plant& plant = block.m_hasPlant.get();
		REQUIRE(!area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		REQUIRE(plant.m_growthEvent.exists());
		Item& bucket = simulation.createItemNongeneric(ItemType::byName("bucket"), MaterialType::byName("poplar wood"), 50u, 0);
		Block& bucketLocation = area.getBlock(7, 7, 2);
		bucket.setLocation(bucketLocation);
		Block& pondLocation = area.getBlock(3, 9, 1);
		pondLocation.setNotSolid();
		pondLocation.addFluid(100, water);
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
		// Wake up if asleep.
		if(!actor.m_mustSleep.isAwake())
			actor.m_mustSleep.wakeUp();
		// Discard drink objective if exists.
		if(actor.m_mustDrink.getVolumeFluidRequested() != 0)
			actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		// Discard eat objective if exists.
		if(actor.m_mustEat.getMassFoodRequested() != 0)
			actor.m_mustEat.eat(actor.m_mustEat.getMassFoodRequested());
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
