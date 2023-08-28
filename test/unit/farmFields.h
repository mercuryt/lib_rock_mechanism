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
TEST_CASE("farm fields")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	Block& block = area.m_blocks[4][4][2];
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(block, block);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
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
		REQUIRE(!block.m_reservable.isFullyReserved(faction));
		const SowSeedsObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(actor));
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
		actor.m_hasObjectives.getNext();
		REQUIRE(actor.m_hasObjectives.hasCurrent());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sow seeds");
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.size() == 1);
		simulation.doStep();
		REQUIRE(block.m_reservable.isFullyReserved(faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		while(actor.m_canMove.getDestination() != nullptr)
			simulation.doStep();
		REQUIRE(actor.m_canMove.getPath().empty());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::SowSeeds));
		const Step eventStep = simulation.m_step + Config::sowSeedsStepsDuration;
		while(simulation.m_step <= eventStep)
			simulation.doStep();
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
		actor.m_hasObjectives.getNext();
		REQUIRE(actor.m_hasObjectives.hasCurrent());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "harvest");
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.size() == 1);
		simulation.doStep();
		REQUIRE(block.m_reservable.isFullyReserved(faction));
		REQUIRE(!actor.m_canMove.getPath().empty());
		while(actor.m_canMove.getDestination() != nullptr)
			simulation.doStep();
		REQUIRE(actor.m_canMove.getPath().empty());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.empty());
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!block.m_hasDesignations.contains(faction, BlockDesignation::Harvest));
		const Step eventStep = simulation.m_step + Config::harvestEventDuration;
		while(simulation.m_step <= eventStep)
			simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!objectiveType.canBeAssigned(actor));
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		Item& item = **block.m_hasItems.getAll().begin();
		REQUIRE(item.m_materialType == MaterialType::byName("plant matter"));
		REQUIRE(item.m_quantity == wheatGrass.harvestData->itemQuantity);
		REQUIRE(item.m_itemType == ItemType::byName("wheat seeds"));
	}
	SUBCASE("give plants fluid")
	{
		static const FluidType& water = FluidType::byName("water");
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		block.m_hasPlant.addPlant(wheatGrass);
		Plant& plant = block.m_hasPlant.get();
		REQUIRE(!area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		REQUIRE(plant.m_growthEvent.exists());
		simulation.m_step = simulation.m_step + wheatGrass.stepsNeedsFluidFrequency;
		simulation.doStep();
		REQUIRE(plant.m_volumeFluidRequested != 0);
		REQUIRE(!plant.m_growthEvent.exists());
		REQUIRE(area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		const GivePlantsFluidObjectiveType objectiveType;
		actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
		REQUIRE(objectiveType.canBeAssigned(actor));
		actor.m_hasObjectives.getNext();
		Item& bucket = simulation.createItem(ItemType::byName("bucket"), MaterialType::byName("poplar wood"), 50u, 0u, nullptr);
		Block& bucketLocation = area.m_blocks[7][7][2];
		bucket.setLocation(bucketLocation);
		Block& pondLocation = area.m_blocks[3][4][2];
		pondLocation.setNotSolid();
		pondLocation.addFluid(100, water);
		REQUIRE(actor.m_canPickup.canPickupAny(bucket));
		simulation.doStep();
		REQUIRE(actor.m_canMove.getDestination() == &bucketLocation);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "give plants fluid");
		while(actor.m_location != &bucketLocation)
			simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.size() == 1);
		REQUIRE(actor.m_canPickup.isCarryingEmptyContainerWhichCanHoldFluid());
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasks.size() == 0);
		REQUIRE(actor.m_canMove.getDestination() != nullptr);
		REQUIRE(actor.m_canMove.getDestination()->isAdjacentTo(pondLocation));
		while(!actor.m_location->isAdjacentTo(pondLocation))
			simulation.doStep();
		REQUIRE(bucket.m_hasCargo.containsAnyFluid());
		REQUIRE(bucket.m_hasCargo.getFluidType() == water);
		REQUIRE(actor.m_canMove.getDestination() == &block);
		while(actor.m_location != &block)
			simulation.doStep();
		Step eventFinishedAt = simulation.m_step + Config::givePlantsFluidDelaySteps;
		while(simulation.m_step <= eventFinishedAt)
			simulation.doStep();
		REQUIRE(plant.m_volumeFluidRequested == 0);
		REQUIRE(plant.m_growthEvent.exists());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "give plants fluid");
	}
}
