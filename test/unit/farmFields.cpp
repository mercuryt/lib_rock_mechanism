#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/plants.h"
#include "../../engine/actors/actors.h"
#include "../../engine/farmFields.h"
#include "../../engine/geometry/cuboid.h"
#include "../../engine/threadedTask.h"
#include "../../engine/faction.h"
#include "../../engine/objectives/sowSeeds.h"
#include "../../engine/objectives/harvest.h"
#include "../../engine/objectives/givePlantsFluid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/items/items.h"
#include "../../engine/numericTypes/types.h"
#include "../../engine/designations.h"
#include "../../engine/objectives/wait.h"
#include "../../engine/path/terrainFacade.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/definitions/materialType.h"
#include <memory>
TEST_CASE("sow")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation("", DateTime(22, PlantSpecies::getDayOfYearForSowStart(wheatGrass), 1200).toSteps());
	FactionId faction = simulation.createFaction("Tower Of Power");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	area.m_blockDesignations.registerFaction(faction);
	BlockIndex fieldLocation = blocks.getIndex_i(4, 4, 2);
	BlockIndex pondLocation = blocks.getIndex_i(8, 8, 1);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	blocks.solid_setNot(pondLocation);
	blocks.fluid_add(pondLocation, Config::maxBlockVolume, FluidType::byName("water"));
	BlockIndex foodLocation = blocks.getIndex_i(8,9,2);
	items.create({.itemType=ItemType::byName("apple"), .materialType=MaterialType::byName("plant matter"), .location=foodLocation, .quantity=Quantity::create(50)});
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(blocks, fieldLocation, fieldLocation);
	FarmField& field = area.m_hasFarmFields.getForFaction(faction).create(area, cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
		.hasCloths=false,
		.hasSidearm=false,
	});
	const SowSeedsObjectiveType& objectiveType = static_cast<const SowSeedsObjectiveType&>(ObjectiveType::getByName("sow seeds"));
	SUBCASE("success")
	{
		CHECK(field.plantSpecies.empty());
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		CHECK(field.plantSpecies == wheatGrass);
		CHECK(blocks.farm_contains(fieldLocation, faction));
		CHECK(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		CHECK(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		CHECK(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		CHECK(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		CHECK(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		CHECK(!blocks.isReserved(fieldLocation, faction));
		CHECK(actors.objective_getCurrentName(actor) == "sow seeds");
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(actors.objective_getCurrent<SowSeedsObjective>(actor).canSowAt(area, fieldLocation, actor));
		constexpr bool detour = false;
		constexpr bool adjacent = true;
		CHECK(!area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(actor)).findPathToWithoutMemo(actors.getLocation(actor), actors.getFacing(actor), actors.getShape(actor), fieldLocation, detour, adjacent, faction).path.empty());
		simulation.doStep();
		CHECK(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		CHECK(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		CHECK(actors.move_getPath(actor).empty());
		CHECK(!actors.move_hasPathRequest(actor));
		CHECK(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		CHECK(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		simulation.fastForward(Config::sowSeedsStepsDuration);
		CHECK(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		CHECK(!area.getPlants().getAll().empty());
		CHECK(!objectiveType.canBeAssigned(area, actor));
		PlantIndex plant = plants.getAll().front();
		CHECK(plants.getPercentGrown(plant) == 0);
		CHECK(plants.getSpecies(plant) == wheatGrass);
		CHECK(actors.objective_getCurrentName(actor) != "sow seeds");
		plants.die(plant);
		CHECK(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		area.m_hasFarmFields.getForFaction(faction).remove(area, field);
		CHECK(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer accessable to sow")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 2), blocks.getIndex_i(8, 3, 2), marble);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 2);
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		CHECK(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		CHECK(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		CHECK(!actors.move_getPath(actor).empty());
		blocks.solid_set(gateway, marble, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "sow seeds");
		CHECK(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer viable to sow")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		blocks.solid_set(blocks.getBlockBelow(fieldLocation), marble, false);
		CHECK(!blocks. plant_canGrowHereAtSomePointToday(fieldLocation, wheatGrass));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, fieldLocation);
		// No alternative route or location found, cannot complete objective.
		CHECK(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "sow seeds");
	}
	SUBCASE("location no longer selected to sow")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		area.m_hasFarmFields.getForFaction(faction).remove(area, field);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, fieldLocation);
		// BlockIndex is not select to grow anymore, cannot complete task, search for another route / another location to sow.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "sow seeds");
	}
	SUBCASE("player cancels sowing objective")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		CHECK(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		actors.objective_cancel(actor, actors.objective_getCurrent<Objective>(actor));
		CHECK(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		CHECK(!blocks.isReserved(fieldLocation, faction));
	}
	SUBCASE("player delays sowing objective")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		simulation.doStep();
		CHECK(!actors.move_getPath(actor).empty());
		CHECK(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(fieldLocation);
		actors.objective_addTaskToStart(actor, std::move(goToObjective));
		CHECK(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		CHECK(!blocks.isReserved(fieldLocation, faction));
	}
}
TEST_CASE("harvest")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation("", DateTime(1, PlantSpecies::getDayOfYearToStartHarvest(wheatGrass), 1200).toSteps());
	FactionId faction = simulation.createFaction("Tower Of Power");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	area.m_blockDesignations.registerFaction(faction);
	BlockIndex block = blocks.getIndex_i(4, 4, 2);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(blocks, block, block);
	FarmField& field = area.m_hasFarmFields.getForFaction(faction).create(area, cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
	});
	const HarvestObjectiveType& objectiveType = static_cast<const HarvestObjectiveType&>(ObjectiveType::getByName("harvest"));
	SUBCASE("harvest")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(plants.getQuantityToHarvest(blocks.plant_get(block)) == PlantSpecies::getItemQuantityToHarvest(wheatGrass));
		CHECK(blocks.designation_has(block, faction, BlockDesignation::Harvest));
		CHECK(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		CHECK(actors.objective_getCurrentName(actor) == "harvest");
		HarvestObjective& objective = actors.objective_getCurrent<HarvestObjective>(actor);
		CHECK(objective.blockContainsHarvestablePlant(area, block, actor));
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(actors.move_canPathTo(actor, block));
		const AreaHasBlockDesignationsForFaction& designations = area.m_blockDesignations.getForFaction(faction);
		uint32_t offset = designations.getOffsetForDesignation(BlockDesignation::Harvest);
		CHECK(designations.checkWithOffset(offset, block));
		simulation.doStep();
		CHECK(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		CHECK(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		CHECK(actors.move_getPath(actor).empty());
		CHECK(!actors.move_hasPathRequest(actor));
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		simulation.fastForward(Config::harvestEventDuration);
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!objectiveType.canBeAssigned(area, actor));
		CHECK(actors.objective_getCurrentName(actor) != "harvest");
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		ItemIndex item = *blocks.item_getAll(actors.getLocation(actor)).begin();
		CHECK(items.getMaterialType(item) == MaterialType::byName("plant matter"));
		CHECK(items.getQuantity(item) + plants.getQuantityToHarvest(blocks.plant_get(block)) == PlantSpecies::getItemQuantityToHarvest(wheatGrass));
		CHECK(items.getItemType(item) == ItemType::byName("wheat seed"));
	}
	SUBCASE("location no longer accessable to harvest")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 2), blocks.getIndex_i(8, 3, 2), marble);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 2);
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!actors.move_getPath(actor).empty());
		blocks.solid_set(gateway, marble, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "harvest");
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("location no longer contains plant")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!actors.move_getPath(actor).empty());
		blocks.plant_erase(block);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "harvest");
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("plant no longer harvestable")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!actors.move_getPath(actor).empty());
		plants.endOfHarvest(blocks.plant_get(block));
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "harvest");
	}
	SUBCASE("player cancels harvest")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		actors.objective_cancel(actor, actors.objective_getCurrent<Objective>(actor));
		CHECK(blocks.designation_has(block, faction, BlockDesignation::Harvest));
	}
	SUBCASE("player delays harvest")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		CHECK(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		CHECK(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		CHECK(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(block);
		actors.objective_addTaskToStart(actor, std::move(goToObjective));
		CHECK(blocks.designation_has(block, faction, BlockDesignation::Harvest));
	}
}
TEST_CASE("givePlantFluid")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation("", DateTime(1, PlantSpecies::getDayOfYearToStartHarvest(wheatGrass) - 1, 1200).toSteps());
	FactionId faction = simulation.createFaction("Tower Of Power");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	area.m_blockDesignations.registerFaction(faction);
	BlockIndex block = blocks.getIndex_i(4, 4, 2);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(blocks, block, block);
	FarmField& field = area.m_hasFarmFields.getForFaction(faction).create(area, cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
	});
	ActorReference actorRef = actors.getReference(actor);
	SUBCASE("give plants fluid")
	{
		static FluidTypeId water = FluidType::byName("water");
		area.m_hasFarmFields.getForFaction(faction).setSpecies(area, field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(50));
		PlantIndex plant = blocks.plant_get(block);
		CHECK(!area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		CHECK(plants.isGrowing(plant));
		BlockIndex bucketLocation = blocks.getIndex_i(7, 7, 2);
		ItemIndex bucket = items.create({.itemType=ItemType::byName("bucket"),.materialType=MaterialType::byName("poplar wood"), .location=bucketLocation, .quality=Quality::create(50u), .percentWear=Percent::create(0)});
		BlockIndex pondLocation = blocks.getIndex_i(3, 9, 1);
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, CollisionVolume::create(100), water);
		CHECK(actors.canPickUp_quantityWhichCanBePickedUpUnencombered(actor, items.getItemType(bucket), items.getMaterialType(bucket)) != 0);
		plants.setMaybeNeedsFluid(plant);
		blocks.plant_exists(block);
		CHECK(plants.getVolumeFluidRequested(plant) != 0);
		CHECK(!plants.isGrowing(plant));
		CHECK(area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		const GivePlantsFluidObjectiveType& objectiveType = static_cast<const GivePlantsFluidObjectiveType&>(ObjectiveType::getByName("give plants fluid"));
		CHECK(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		// Create project, verifiy access to bucket and pond.
		simulation.doStep();
		actors.satisfyNeeds(actor);
		CHECK(actors.objective_getCurrentName(actor) == "give plants fluid");
		const GivePlantsFluidObjective& objective = actors.objective_getCurrent<GivePlantsFluidObjective>(actor);
		CHECK(objective.hasProject());
		const GivePlantFluidProject& project = objective.getProject();
		CHECK(project.reservationsComplete());
		CHECK(project.hasTryToHaulThreadedTask());
		// Select haul strategy.
		simulation.doStep();
		const ProjectWorker& projectWorker = project.getProjectWorkerFor(actorRef);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Make Path.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) == "give plants fluid");
		CHECK(actors.move_getDestination(actor).exists());
		CHECK(items.isAdjacentToLocation(bucket, actors.move_getDestination(actor)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, bucketLocation);
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(actors.canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(actor));
		CHECK(actors.move_getDestination(actor).empty());
		simulation.doStep();
		CHECK(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		CHECK(actors.move_getDestination(actor).exists());
		CHECK(blocks.isAdjacentToIncludingCornersAndEdges(actors.move_getDestination(actor), pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, pondLocation);
		CHECK(items.cargo_containsAnyFluid(bucket));
		CHECK(items.cargo_getFluidType(bucket) == water);
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(actors.move_getDestination(actor).empty());
		CHECK(actors.move_getPath(actor).empty());
		simulation.doStep();
		CHECK(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		CHECK(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		simulation.fastForward(project.getDuration());
		CHECK(plants.getVolumeFluidRequested(plant) == 0);
		CHECK(plants.isGrowing(plant));
		CHECK(actors.objective_getCurrentName(actor) != "give plants fluid");
	}
}