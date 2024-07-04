#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/plants.h"
#include "../../engine/actors/actors.h"
#include "../../engine/farmFields.h"
#include "../../engine/cuboid.h"
#include "../../engine/threadedTask.h"
#include "../../engine/faction.h"
#include "../../engine/objectives/sowSeeds.h"
#include "../../engine/objectives/harvest.h"
#include "../../engine/objectives/givePlantsFluid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/itemType.h"
#include "../../engine/items/items.h"
#include "../../engine/itemType.h"
#include "../../engine/materialType.h"
#include "../../engine/types.h"
#include <memory>
TEST_CASE("sow")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	uint16_t dayBeforeSowingStarts = wheatGrass.dayOfYearForSowStart - 1u;
	Simulation simulation(L"", DateTime(10, dayBeforeSowingStarts, 1200).toSteps());
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	area.m_blockDesignations.registerFaction(faction);
	BlockIndex fieldLocation = blocks.getIndex({4, 4, 2});
	BlockIndex pondLocation = blocks.getIndex({8,8,2});
	blocks.solid_setNot(pondLocation);
	blocks.fluid_add(pondLocation, Config::maxBlockVolume, FluidType::byName("water"));
	BlockIndex foodLocation = blocks.getIndex({8,9,2});
	items.create({.itemType=ItemType::byName("apple"), .materialType=MaterialType::byName("plant matter"), .location=foodLocation, .quantity=50});
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(blocks, fieldLocation, fieldLocation);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf, 
		.location=blocks.getIndex({1, 1, 2}),
		.faction=&faction,
		.hasCloths=false,
		.hasSidearm=false,
	});
	SUBCASE("success")
	{
		REQUIRE(field.plantSpecies == nullptr);
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		REQUIRE(field.plantSpecies == &wheatGrass);
		REQUIRE(blocks.farm_contains(fieldLocation, faction));
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		const SowSeedsObjectiveType objectiveType;
		// Skip ahead to planting time.
		simulation.fastForward(Config::stepsPerDay);
		REQUIRE(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(!blocks.isReserved(fieldLocation, faction));
		REQUIRE(actors.objective_getCurrentName(actor) == "sow seeds");
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		REQUIRE(actors.objective_getCurrent<SowSeedsObjective>(actor).canSowAt(area, fieldLocation));
		REQUIRE(!area.m_hasTerrainFacades.at(actors.getMoveType(actor)).findPathAdjacentToAndUnreserved(actors.getLocation(actor), actors.getShape(actor), actors.getFacing(actor), fieldLocation, faction).path.empty());
		simulation.doStep();
		REQUIRE(blocks.isReserved(fieldLocation, faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		REQUIRE(actors.move_getPath(actor).empty());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		simulation.fastForward(Config::sowSeedsStepsDuration);
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!area.getPlants().getAll().empty());
		REQUIRE(!objectiveType.canBeAssigned(area, actor));
		PlantIndex plant = plants.getAll().front();
		REQUIRE(plants.getPercentGrown(plant) == 0);
		REQUIRE(plants.getSpecies(plant) == wheatGrass);
		REQUIRE(actors.objective_getCurrentName(actor) != "sow seeds");
		plants.die(plant);
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		area.m_hasFarmFields.at(faction).remove(field);
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer accessable to sow")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 3, 2}), blocks.getIndex({8, 3, 2}), marble);
		BlockIndex gateway = blocks.getIndex({9, 3, 2});
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.solid_set(gateway, marble, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != "sow seeds");
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer viable to sow")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.solid_set(blocks.getBlockBelow(fieldLocation), marble, false);
		REQUIRE(!blocks. plant_canGrowHereAtSomePointToday(fieldLocation, wheatGrass));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, fieldLocation);
		// No alternative route or location found, cannot complete objective.
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != "sow seeds");
	}
	SUBCASE("location no longer selected to sow")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		area.m_hasFarmFields.at(faction).remove(field);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, fieldLocation);
		// BlockIndex is not select to grow anymore, cannot complete task, search for another route / another location to sow.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != "sow seeds");
	}
	SUBCASE("player cancels sowing objective")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		actors.objective_cancel(actor, actors.objective_getCurrent<Objective>(actor));
		REQUIRE(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(!blocks.isReserved(fieldLocation, faction));
	}
	SUBCASE("player delays sowing objective")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		// Skip ahead to planting time.
		simulation.fastForwardUntill({1, wheatGrass.dayOfYearForSowStart, 1200 });
		const SowSeedsObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(actor, fieldLocation);
		actors.objective_addTaskToStart(actor, std::move(goToObjective));
		REQUIRE(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(!blocks.isReserved(fieldLocation, faction));
	}
}
TEST_CASE("harvest")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	uint16_t dayBeforeHarvest = wheatGrass.harvestData->dayOfYearToStart - 1u;
	Simulation simulation(L"", DateTime(1, dayBeforeHarvest, 1200).toSteps());
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	area.m_blockDesignations.registerFaction(faction);
	BlockIndex block = blocks.getIndex({4, 4, 2});
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(blocks, block, block);
	FarmField& field = area.m_hasFarmFields.at(faction).create(cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf, 
		.location=blocks.getIndex({1, 1, 2}),
		.faction=&faction,
	});
	SUBCASE("harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, 100);
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(plants.getQuantityToHarvest(blocks.plant_get(block)) == wheatGrass.harvestData->itemQuantity);
		const HarvestObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		REQUIRE(actors.objective_getCurrentName(actor) == "harvest");
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		simulation.doStep();
		REQUIRE(blocks.isReserved(block, faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		REQUIRE(actors.move_getPath(actor).empty());
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		simulation.fastForward(Config::harvestEventDuration);
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!objectiveType.canBeAssigned(area, actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		ItemIndex item = *blocks.item_getAll(actors.getLocation(actor)).begin();
		REQUIRE(items.getMaterialType(item) == MaterialType::byName("plant matter"));
		REQUIRE(items.getQuantity(item) + plants.getQuantityToHarvest(blocks.plant_get(block)) == wheatGrass.harvestData->itemQuantity);
		REQUIRE(items.getItemType(item) == ItemType::byName("wheat seed"));
	}
	SUBCASE("location no longer accessable to harvest")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 3, 2}), blocks.getIndex({8, 3, 2}), marble);
		BlockIndex gateway = blocks.getIndex({9, 3, 2});
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.solid_set(gateway, marble, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != "harvest");
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("location no longer contains plant")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.plant_erase(block);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != "harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("plant no longer harvestable")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		plants.endOfHarvest(blocks.plant_get(block));
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != "harvest");
	}
	SUBCASE("player cancels harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		actors.objective_cancel(actor, actors.objective_getCurrent<Objective>(actor));
		REQUIRE(blocks.designation_has(block, faction, BlockDesignation::Harvest));
	}
	SUBCASE("player delays harvest")
	{
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, 100);
		// Skip ahead to harvest time.
		simulation.fastForwardUntill({1, wheatGrass.harvestData->dayOfYearToStart, 1200});
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		const HarvestObjectiveType objectiveType;
		actors.objective_setPriority(actor, objectiveType, 10);
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(actor, block);
		actors.objective_addTaskToStart(actor, std::move(goToObjective));
		REQUIRE(blocks.designation_has(block, faction, BlockDesignation::Harvest));
	}
	SUBCASE("give plants fluid")
	{
		static const FluidType& water = FluidType::byName("water");
		area.m_hasFarmFields.at(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass);
		PlantIndex plant = blocks.plant_get(block);
		REQUIRE(!area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		REQUIRE(plants.isGrowing(plant));
		BlockIndex bucketLocation = blocks.getIndex({7, 7, 2});
		ItemIndex bucket = items.create({.itemType=ItemType::byName("bucket"),.materialType=MaterialType::byName("poplar wood"), .location=bucketLocation, .quality=50u, .percentWear=0});
		BlockIndex pondLocation = blocks.getIndex({3, 9, 1});
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, 100, water);
		REQUIRE(actors.canPickUp_quantityWhichCanBePickedUpUnencombered(actor, items.getItemType(bucket), items.getMaterialType(bucket)));
		plants.setMaybeNeedsFluid(plant);
		blocks.plant_exists(block);
		REQUIRE(plants.getVolumeFluidRequested(plant) != 0);
		REQUIRE(!plants.isGrowing(plant));
		REQUIRE(area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		const GivePlantsFluidObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType, 10);
		simulation.doStep();
		actors.satisfyNeeds(actor);
		REQUIRE(actors.objective_getCurrentName(actor) == "give plants fluid");
		simulation.doStep();
		REQUIRE(items.isAdjacentToLocation(bucket, actors.move_getDestination(actor)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, bucketLocation);
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		REQUIRE(actors.canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(actor));
		REQUIRE(actors.move_getDestination(actor) == BLOCK_INDEX_MAX);
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		REQUIRE(actors.move_getDestination(actor) != BLOCK_INDEX_MAX);
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(actors.move_getDestination(actor), pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, pondLocation);
		REQUIRE(items.cargo_containsAnyFluid(bucket));
		REQUIRE(items.cargo_getFluidType(bucket) == water);
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 1);
		REQUIRE(actors.move_getDestination(actor) == BLOCK_INDEX_MAX);
		REQUIRE(actors.move_getPath(actor).empty());
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		REQUIRE(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		simulation.fastForward(Config::givePlantsFluidDelaySteps);
		REQUIRE(plants.getVolumeFluidRequested(plant) == 0);
		REQUIRE(plants.isGrowing(plant));
		REQUIRE(actors.objective_getCurrentName(actor) != "give plants fluid");
	}
}
