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
#include "designations.h"
#include "objectives/wait.h"
#include "terrainFacade.h"
#include <memory>
TEST_CASE("sow")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName(L"wheat grass");
	static MaterialTypeId dirt = MaterialType::byName(L"dirt");
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation(L"", DateTime(22, PlantSpecies::getDayOfYearForSowStart(wheatGrass), 1200).toSteps());
	FactionId faction = simulation.createFaction(L"Tower Of Power");
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
	blocks.fluid_add(pondLocation, Config::maxBlockVolume, FluidType::byName(L"water"));
	BlockIndex foodLocation = blocks.getIndex_i(8,9,2);
	items.create({.itemType=ItemType::byName(L"apple"), .materialType=MaterialType::byName(L"plant matter"), .location=foodLocation, .quantity=Quantity::create(50)});
	area.m_hasFarmFields.registerFaction(faction);
	Cuboid cuboid(blocks, fieldLocation, fieldLocation);
	FarmField& field = area.m_hasFarmFields.getForFaction(faction).create(cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
		.hasCloths=false,
		.hasSidearm=false,
	});
	const SowSeedsObjectiveType& objectiveType = static_cast<const SowSeedsObjectiveType&>(ObjectiveType::getByName(L"sow seeds"));
	SUBCASE("success")
	{
		REQUIRE(field.plantSpecies.empty());
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		REQUIRE(field.plantSpecies == wheatGrass);
		REQUIRE(blocks.farm_contains(fieldLocation, faction));
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(!blocks.isReserved(fieldLocation, faction));
		REQUIRE(actors.objective_getCurrentName(actor) == L"sow seeds");
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(actors.objective_getCurrent<SowSeedsObjective>(actor).canSowAt(area, fieldLocation, actor));
		constexpr bool detour = false;
		constexpr bool adjacent = true;
		REQUIRE(!area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(actor)).findPathToWithoutMemo(actors.getLocation(actor), actors.getFacing(actor), actors.getShape(actor), fieldLocation, detour, adjacent, faction).path.empty());
		simulation.doStep();
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		REQUIRE(actors.move_getPath(actor).empty());
		REQUIRE(!actors.move_hasPathRequest(actor));
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		simulation.fastForward(Config::sowSeedsStepsDuration);
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!area.getPlants().getAll().empty());
		REQUIRE(!objectiveType.canBeAssigned(area, actor));
		PlantIndex plant = plants.getAll().front();
		REQUIRE(plants.getPercentGrown(plant) == 0);
		REQUIRE(plants.getSpecies(plant) == wheatGrass);
		REQUIRE(actors.objective_getCurrentName(actor) != L"sow seeds");
		plants.die(plant);
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		area.m_hasFarmFields.getForFaction(faction).remove(field);
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer accessable to sow")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 2), blocks.getIndex_i(8, 3, 2), marble);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 2);
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.solid_set(gateway, marble, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != L"sow seeds");
		REQUIRE(area.m_hasFarmFields.hasSowSeedsDesignations(faction));
	}
	SUBCASE("location no longer viable to sow")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.solid_set(blocks.getBlockBelow(fieldLocation), marble, false);
		REQUIRE(!blocks. plant_canGrowHereAtSomePointToday(fieldLocation, wheatGrass));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, fieldLocation);
		// No alternative route or location found, cannot complete objective.
		REQUIRE(!area.m_hasFarmFields.hasSowSeedsDesignations(faction));
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != L"sow seeds");
	}
	SUBCASE("location no longer selected to sow")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		area.m_hasFarmFields.getForFaction(faction).remove(field);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, fieldLocation);
		// BlockIndex is not select to grow anymore, cannot complete task, search for another route / another location to sow.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != L"sow seeds");
	}
	SUBCASE("player cancels sowing objective")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
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
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		simulation.doStep();
		REQUIRE(!actors.move_getPath(actor).empty());
		REQUIRE(!blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(fieldLocation);
		actors.objective_addTaskToStart(actor, std::move(goToObjective));
		REQUIRE(blocks.designation_has(fieldLocation, faction, BlockDesignation::SowSeeds));
		REQUIRE(!blocks.isReserved(fieldLocation, faction));
	}
}
TEST_CASE("harvest")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName(L"wheat grass");
	static MaterialTypeId dirt = MaterialType::byName(L"dirt");
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation(L"", DateTime(1, PlantSpecies::getDayOfYearToStartHarvest(wheatGrass), 1200).toSteps());
	FactionId faction = simulation.createFaction(L"Tower Of Power");
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
	FarmField& field = area.m_hasFarmFields.getForFaction(faction).create(cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
	});
	const HarvestObjectiveType& objectiveType = static_cast<const HarvestObjectiveType&>(ObjectiveType::getByName(L"harvest"));
	SUBCASE("harvest")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(plants.getQuantityToHarvest(blocks.plant_get(block)) == PlantSpecies::getItemQuantityToHarvest(wheatGrass));
		REQUIRE(blocks.designation_has(block, faction, BlockDesignation::Harvest));
		REQUIRE(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		REQUIRE(actors.objective_getCurrentName(actor) == L"harvest");
		HarvestObjective& objective = actors.objective_getCurrent<HarvestObjective>(actor);
		REQUIRE(objective.blockContainsHarvestablePlant(area, block, actor));
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(actors.move_canPathTo(actor, block));
		const AreaHasBlockDesignationsForFaction& designations = area.m_blockDesignations.getForFaction(faction);
		uint32_t offset = designations.getOffsetForDesignation(BlockDesignation::Harvest);
		REQUIRE(designations.checkWithOffset(offset, block));
		simulation.doStep();
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		REQUIRE(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		REQUIRE(actors.move_getPath(actor).empty());
		REQUIRE(!actors.move_hasPathRequest(actor));
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		simulation.fastForward(Config::harvestEventDuration);
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!objectiveType.canBeAssigned(area, actor));
		REQUIRE(actors.objective_getCurrentName(actor) != L"harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		ItemIndex item = *blocks.item_getAll(actors.getLocation(actor)).begin();
		REQUIRE(items.getMaterialType(item) == MaterialType::byName(L"plant matter"));
		REQUIRE(items.getQuantity(item) + plants.getQuantityToHarvest(blocks.plant_get(block)) == PlantSpecies::getItemQuantityToHarvest(wheatGrass));
		REQUIRE(items.getItemType(item) == ItemType::byName(L"wheat seed"));
	}
	SUBCASE("location no longer accessable to harvest")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 2), blocks.getIndex_i(8, 3, 2), marble);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 2);
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.solid_set(gateway, marble, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != L"harvest");
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("location no longer contains plant")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		blocks.plant_erase(block);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != L"harvest");
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
	}
	SUBCASE("plant no longer harvestable")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!actors.move_getPath(actor).empty());
		plants.endOfHarvest(blocks.plant_get(block));
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, block);
		// No alternative route or location found, cannot complete objective.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) != L"harvest");
	}
	SUBCASE("player cancels harvest")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		actors.objective_cancel(actor, actors.objective_getCurrent<Objective>(actor));
		REQUIRE(blocks.designation_has(block, faction, BlockDesignation::Harvest));
	}
	SUBCASE("player delays harvest")
	{
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(100));
		REQUIRE(area.m_hasFarmFields.hasHarvestDesignations(faction));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		actors.satisfyNeeds(actor);
		simulation.doStep();
		REQUIRE(!area.m_hasFarmFields.hasHarvestDesignations(faction));
		REQUIRE(!blocks.designation_has(block, faction, BlockDesignation::Harvest));
		std::unique_ptr<GoToObjective> goToObjective = std::make_unique<GoToObjective>(block);
		actors.objective_addTaskToStart(actor, std::move(goToObjective));
		REQUIRE(blocks.designation_has(block, faction, BlockDesignation::Harvest));
	}
}
TEST_CASE("givePlantFluid")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName(L"wheat grass");
	static MaterialTypeId dirt = MaterialType::byName(L"dirt");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation(L"", DateTime(1, PlantSpecies::getDayOfYearToStartHarvest(wheatGrass) - 1, 1200).toSteps());
	FactionId faction = simulation.createFaction(L"Tower Of Power");
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
	FarmField& field = area.m_hasFarmFields.getForFaction(faction).create(cuboid);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
	});
	SUBCASE("give plants fluid")
	{
		static FluidTypeId water = FluidType::byName(L"water");
		area.m_hasFarmFields.getForFaction(faction).setSpecies(field, wheatGrass);
		blocks.plant_create(block, wheatGrass, Percent::create(50));
		PlantIndex plant = blocks.plant_get(block);
		REQUIRE(!area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		REQUIRE(plants.isGrowing(plant));
		BlockIndex bucketLocation = blocks.getIndex_i(7, 7, 2);
		ItemIndex bucket = items.create({.itemType=ItemType::byName(L"bucket"),.materialType=MaterialType::byName(L"poplar wood"), .location=bucketLocation, .quality=Quality::create(50u), .percentWear=Percent::create(0)});
		BlockIndex pondLocation = blocks.getIndex_i(3, 9, 1);
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, CollisionVolume::create(100), water);
		REQUIRE(actors.canPickUp_quantityWhichCanBePickedUpUnencombered(actor, items.getItemType(bucket), items.getMaterialType(bucket)) != 0);
		plants.setMaybeNeedsFluid(plant);
		blocks.plant_exists(block);
		REQUIRE(plants.getVolumeFluidRequested(plant) != 0);
		REQUIRE(!plants.isGrowing(plant));
		REQUIRE(area.m_hasFarmFields.hasGivePlantsFluidDesignations(faction));
		const GivePlantsFluidObjectiveType& objectiveType = static_cast<const GivePlantsFluidObjectiveType&>(ObjectiveType::getByName(L"give plants fluid"));
		REQUIRE(objectiveType.canBeAssigned(area, actor));
		actors.objective_setPriority(actor, objectiveType.getId(), Priority::create(10));
		simulation.doStep();
		actors.satisfyNeeds(actor);
		REQUIRE(actors.objective_getCurrentName(actor) == L"give plants fluid");
		[[maybe_unused]] const GivePlantsFluidObjective& objective = actors.objective_getCurrent<GivePlantsFluidObjective>(actor);
		REQUIRE(objective.getPlantLocation().exists());
		REQUIRE(!objective.itemExists());
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(objective.canGetFluidHaulingItemAt(area, bucketLocation, actor));
		simulation.doStep();
		REQUIRE(objective.itemExists());
		REQUIRE(items.isAdjacentToLocation(bucket, actors.move_getDestination(actor)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, bucketLocation);
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(actors.canPickUp_isCarryingEmptyContainerWhichCanHoldFluid(actor));
		REQUIRE(actors.move_getDestination(actor).empty());
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		REQUIRE(actors.move_getDestination(actor).exists());
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(actors.move_getDestination(actor), pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, pondLocation);
		REQUIRE(items.cargo_containsAnyFluid(bucket));
		REQUIRE(items.cargo_getFluidType(bucket) == water);
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(actors.move_getDestination(actor).empty());
		REQUIRE(actors.move_getPath(actor).empty());
		simulation.doStep();
		REQUIRE(simulation.m_threadedTaskEngine.m_tasksForNextStep.size() == 0);
		REQUIRE(!actors.move_getPath(actor).empty());
		BlockIndex destination = actors.move_getDestination(actor);
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		simulation.fastForward(Config::givePlantsFluidDelaySteps);
		REQUIRE(plants.getVolumeFluidRequested(plant) == 0);
		REQUIRE(plants.isGrowing(plant));
		REQUIRE(actors.objective_getCurrentName(actor) != L"give plants fluid");
	}
}