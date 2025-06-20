#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/plants.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/objectives/eat.h"
#include "../../engine/objectives/station.h"
#include "numericTypes/types.h"
TEST_CASE("basicNeedsSentient")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation;
	FactionId faction = simulation.createFaction("Tower Of Power");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=dwarf,
		.percentGrown=Percent::create(50),
		.location=blocks.getIndex_i(1, 1, 2),
		.percentHunger=Percent::create(0),
		.percentTired=Percent::create(0),
		.percentThirst=Percent::create(0),
	});
	actors.setFaction(actor, faction);
	CHECK(actors.grow_isGrowing(actor));
	SUBCASE("drink from pond")
	{
		BlockIndex pondLocation = blocks.getIndex_i(3, 3, 1);
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, CollisionVolume::create(100), water);
		actors.drink_setNeedsFluid(actor);
		CHECK(!actors.grow_isGrowing(actor));
		CHECK(actors.drink_isThirsty(actor));
		CHECK(actors.objective_getCurrentName(actor) == "drink");
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		CHECK(destination.exists());
		CHECK(blocks.isAdjacentToIncludingCornersAndEdges(destination, pondLocation));
		simulation.fastForwardUntillActorIsAtDestination(area, actor, destination);
		simulation.fastForward(Config::stepsToDrink);
		CHECK(!actors.drink_isThirsty(actor));
		CHECK(actors.objective_getCurrentName(actor) != "drink");
		CollisionVolume drinkVolume = MustDrink::drinkVolumeFor(area, actor);
		simulation.doStep(); // Give a step for the fluid removal to take effect.
		CHECK(blocks.fluid_volumeOfTypeContains(pondLocation, water) == Config::maxBlockVolume - drinkVolume);
	}
	SUBCASE("drink from bucket")
	{
		BlockIndex bucketLocation = blocks.getIndex_i(7, 7, 2);
		ItemIndex bucket = items.create({
			.itemType=ItemType::byName("bucket"),
			.materialType=MaterialType::byName("poplar wood"),
			.location=bucketLocation,
			.quality=Quality::create(50),
			.percentWear=Percent::create(0),
		});
		items.cargo_addFluid(bucket, water, CollisionVolume::create(2));
		actors.drink_setNeedsFluid(actor);
		CHECK(!actors.grow_isGrowing(actor));
		CHECK(actors.drink_isThirsty(actor));
		CHECK(actors.objective_getCurrentName(actor) == "drink");
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		CHECK(destination.exists());
		CHECK(blocks.isAdjacentToIncludingCornersAndEdges(destination, bucketLocation));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, bucket);
		simulation.fastForward(Config::stepsToDrink);
		CHECK(!actors.drink_isThirsty(actor));
		CHECK(actors.objective_getCurrentName(actor) != "drink");
		CollisionVolume drinkVolume = MustDrink::drinkVolumeFor(area, actor);
		CHECK(items.cargo_getFluidVolume(bucket) == CollisionVolume::create(2) - drinkVolume);
	}
	SUBCASE("drink tea")
	{
		//TODO
	}
	SUBCASE("eat prepared meal")
	{
		BlockIndex mealLocation = blocks.getIndex_i(5, 5, 2);
		ItemIndex meal = items.create({
			.itemType=ItemType::byName("prepared meal"),
			.materialType=MaterialType::byName("fruit"),
			.location=mealLocation,
			.quality=Quality::create(50),
			.percentWear=Percent::create(0),
		});
		CHECK(actors.eat_getDesireToEatSomethingAt(actor, mealLocation) == maxRankedEatDesire);
		actors.eat_setIsHungry(actor);
		CHECK(actors.eat_hasObjective(actor));
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(actors.objective_getCurrentName(actor) == "eat");
		CHECK(actors.eat_isHungry(actor));
		simulation.doStep();
		CHECK(!actors.move_hasPathRequest(actor));
		BlockIndex destination = actors.move_getDestination(actor);
		CHECK(destination.exists());
		CHECK(blocks.isAdjacentToIncludingCornersAndEdges(destination, mealLocation));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, meal);
		CHECK(actors.objective_getCurrentName(actor) == "eat");
		CHECK(actors.objective_getCurrent<EatObjective>(actor).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		CHECK(!actors.eat_isHungry(actor));
		CHECK(actors.objective_getCurrentName(actor) != "eat");
		CHECK(blocks.item_empty(mealLocation));
	}
	SUBCASE("eat fruit")
	{
		BlockIndex fruitLocation = blocks.getIndex_i(6, 5, 2);
		ItemIndex fruit = items.create({
			.itemType=ItemType::byName("apple"),
			.materialType=MaterialType::byName("fruit"),
			.location=fruitLocation,
			.quantity=Quantity::create(50),
		});
		CHECK(actors.eat_getDesireToEatSomethingAt(actor, fruitLocation) == 2);
		actors.eat_setIsHungry(actor);
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		CHECK(actors.eat_getPercentStarved(actor) == 0);
		CHECK(actors.eat_getMinimumAcceptableDesire(actor) == 3);
		CHECK(actors.objective_getCurrentName(actor) == "eat");
		NeedType eatNeedType = actors.objective_getCurrent<EatObjective>(actor).getNeedType();
		// Search for acceptable food but fail to find.
		// Supress need.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "eat");
		CHECK(actors.objective_hasSupressedNeed(actor, eatNeedType));
		Step delayRemaning = actors.objective_getNeedDelayRemaining(actor, eatNeedType);
		simulation.fasterForward(delayRemaning);
		CHECK(!actors.objective_hasSupressedNeed(actor, eatNeedType));
		CHECK(actors.objective_getCurrentName(actor) == "eat");
		CHECK(actors.eat_getMinimumAcceptableDesire(actor) == 3);
		simulation.doStep();
		CHECK(actors.objective_hasSupressedNeed(actor, eatNeedType));
		CHECK(actors.objective_getCurrentName(actor) == "wander");
		simulation.fasterForwardUntillPredicate([&](){ return actors.eat_getMinimumAcceptableDesire(actor) == 2 && actors.objective_getCurrentName(actor) == "eat"; }, Config::minutesPerHour * 6);
		CHECK(actors.move_hasPathRequest(actor));
		const EatObjective& objective = actors.objective_getCurrent<EatObjective>(actor);
		CHECK(actors.eat_canEatItem(actor, fruit));
		CHECK(objective.canEatAt(area, fruitLocation, actor));
		CHECK(!objective.hasLocation());
		simulation.doStep();
		// Because we aren't finding a max desirable target we can't use a path generated from the first step of searching, we need a second step to find a path to the selected candidate.
		CHECK(actors.move_hasPathRequest(actor));
		CHECK(objective.hasLocation());
		simulation.doStep();
		CHECK(!actors.move_hasPathRequest(actor));
		CHECK(actors.eat_isHungry(actor));
		CHECK(actors.objective_getCurrentName(actor) == "eat");
		CHECK(actors.move_getDestination(actor).exists());
		CHECK(items.isAdjacentToLocation(fruit, actors.move_getDestination(actor)));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, fruit);
		CHECK(actors.objective_getCurrent<EatObjective>(actor).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		CHECK(!actors.eat_isHungry(actor));
		CHECK(actors.objective_getCurrentName(actor) != "eat");
		CHECK(!blocks.item_empty(fruitLocation));
		CHECK(items.getQuantity(fruit) < 50u);
	}
}
TEST_CASE("basicNeedsNonsentient")
{
	static PlantSpeciesId wheatGrass = PlantSpecies::byName("wheat grass");
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId redDeer = AnimalSpecies::byName("red deer");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=redDeer,
		.percentGrown=Percent::create(50),
		.location=blocks.getIndex_i(1, 1, 2),
	});
	CHECK(actors.grow_isGrowing(actor));
	CHECK(actors.drink_hasThristEvent(actor));
	BlockIndex pondLocation = blocks.getIndex_i(3, 3, 1);
	blocks.solid_setNot(pondLocation);
	blocks.fluid_add(pondLocation, CollisionVolume::create(100), water);
	SUBCASE("sleep outside at current location")
	{
		// Generate objectives, discard eat if it exists.
		actors.sleep_makeTired(actor);
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		actors.stamina_spend(actor, Stamina::create(1));
		CHECK(!actors.stamina_isFull(actor));
		if(actors.eat_getMassFoodRequested(actor) != 0)
			actors.eat_do(actor, actors.eat_getMassFoodRequested(actor));
		CHECK(actors.objective_getCurrentName(actor) == "sleep");
		CHECK(actors.sleep_isAwake(actor));
		// Look for sleeping spot.
		simulation.doStep();
		// No spot found better then current one.
		CHECK(actors.move_getDestination(actor).empty());
		CHECK(!actors.sleep_isAwake(actor));
		// Wait for wake up.
		simulation.fasterForward(AnimalSpecies::getStepsSleepDuration(redDeer));
		CHECK(actors.sleep_isAwake(actor));
		CHECK(actors.sleep_hasTiredEvent(actor));
		CHECK(actors.stamina_isFull(actor));
	}
	SUBCASE("eat leaves")
	{
		BlockIndex grassLocation = blocks.getIndex_i(5, 5, 2);
		blocks.plant_create(grassLocation, wheatGrass, Percent::create(100));
		PlantIndex grass = blocks.plant_get(grassLocation);
		// Generate objectives.
		actors.eat_setIsHungry(actor);
		CHECK(actors.objective_getCurrentName(actor) == "eat");
		CHECK(actors.eat_isHungry(actor));
		CHECK(plants.getPercentFoliage(grass) == 100);
		// The deer is wandering randomly while we fast forward until it is hungry, it might or might not already be next to the grass.
		if(!actors.isAdjacentToLocation(actor, grassLocation))
		{
			// Find grass.
			CHECK(actors.move_hasPathRequest(actor));
			simulation.doStep();
			CHECK(!actors.move_hasPathRequest(actor));
			CHECK(actors.objective_getCurrentName(actor) == "eat");
			BlockIndex destination = actors.move_getDestination(actor);
			CHECK(destination.exists());
			CHECK(blocks.isAdjacentToIncludingCornersAndEdges(destination, grassLocation));
			// Go to grass.
			simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, grassLocation);
		}
		// Eat grass.
		simulation.fastForward(Config::stepsToEat);
		CHECK(!actors.eat_isHungry(actor));
		CHECK(actors.objective_getCurrentName(actor) != "eat");
		CHECK(plants.getPercentFoliage(grass) != 100);
	}
	SUBCASE("sleep at assigned spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 2, 2), blocks.getIndex_i(8, 2, 2), dirt);
		BlockIndex gateway = blocks.getIndex_i(9, 2, 2);
		BlockIndex spot = blocks.getIndex_i(5, 5, 2);
		actors.sleep_setSpot(actor, spot);
		actors.sleep_makeTired(actor);
		CHECK(actors.objective_getCurrentName(actor) == "sleep");
		// Path to spot.
		simulation.doStep();
		CHECK(actors.move_getDestination(actor) == spot);
		SUBCASE("success")
		{
			simulation.fastForwardUntillActorIsAtDestination(area, actor, spot);
			// Wait for wake up.
			simulation.fasterForward(AnimalSpecies::getStepsSleepDuration(redDeer));
			CHECK(actors.sleep_isAwake(actor));
			CHECK(actors.sleep_hasTiredEvent(actor));
		}
		SUBCASE("spot no longer suitable")
		{
			blocks.solid_set(spot, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, spot);
			// The actor is unable to path into the destination, and so tries to create another path.
			CHECK(actors.move_hasPathRequest(actor));
			simulation.doStep();
			// Actor chooses current location as new sleeping spot.
			CHECK(!actors.move_hasPathRequest(actor));
			CHECK(!actors.move_hasEvent(actor));
			CHECK(actors.objective_getCurrentName(actor) == "sleep");
			CHECK(actors.sleep_getSpot(actor).exists());
			CHECK(!actors.sleep_isAwake(actor));
		}
		SUBCASE("cannot path to spot")
		{
			blocks.solid_set(gateway, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
			CHECK(actors.move_hasPathRequest(actor));
			// Path to designated spot blocked, try to repath, no path found, clear designated spot.
			simulation.doStep();
			CHECK(actors.sleep_getSpot(actor).empty());
			CHECK(actors.objective_getCurrentName(actor) == "sleep");
			CHECK(actors.move_hasPathRequest(actor));
		}
	}
	SUBCASE("try to find a spot inside")
	{
		//TODO
	}
	SUBCASE("scavenge corpse")
	{
		AnimalSpeciesId blackBear = AnimalSpecies::byName("black bear");
		ActorIndex bear = actors.create(ActorParamaters{
			.species=blackBear,
			.location=blocks.getIndex_i(5, 1, 2),
		});
		ActorIndex deer = actor;
		Mass deerMass = actors.getMass(deer);
		actors.die(deer, CauseOfDeath::thirst);
		CHECK(!actors.isAlive(deer));
		CHECK(actors.eat_getDesireToEatSomethingAt(bear, actors.getLocation(deer)) != 0);
		actors.eat_setIsHungry(bear);
		// Bear is hungry.
		CHECK(actors.eat_getMassFoodRequested(bear) != 0);
		CHECK(actors.objective_getCurrentName(bear) == "eat");
		CHECK(actors.move_hasPathRequest(bear));
		FindPathResult result = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(bear)).findPathToWithoutMemo(actors.getLocation(bear), actors.getFacing(bear), actors.getShape(bear), actors.getLocation(deer));
		CHECK(!result.path.empty());
		EatObjective& objective = actors.objective_getCurrent<EatObjective>(bear);
		CHECK(!objective.hasLocation());
		// Bear goes to deer corpse.
		simulation.doStep();
		CHECK(objective.hasLocation());
		if(!actors.isAdjacentToActor(bear, deer))
			simulation.fastForwardUntillActorIsAdjacentToActor(area, bear, deer);
		// Bear eats.
		simulation.fastForward(Config::stepsToEat);
		CHECK(!actors.eat_isHungry(bear));
		CHECK(actors.objective_getCurrentName(bear) != "eat");
		CHECK(actors.getMass(deer) != deerMass);
	}
	SUBCASE("leave location with unsafe temperature")
	{
		CHECK(actors.grow_isGrowing(actor));
		BlockIndex temperatureSourceLocation = blocks.getIndex_i(1, 1, 3);
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, TemperatureDelta::create(200));
		simulation.doStep();
		CHECK(!actors.temperature_isSafeAtCurrentLocation(actor));
		CHECK(blocks.temperature_get(actors.getLocation(actor)) > AnimalSpecies::getMaximumSafeTemperature(actors.getSpecies(actor)));
		CHECK(actors.objective_getCurrentName(actor) == "get to safe temperature");
		CHECK(!actors.grow_isGrowing(actor));
	}
}
TEST_CASE("actorGrowth")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static FluidTypeId water = FluidType::byName("water");
	Step step = DateTime(8, 60, 10000).toSteps();
	Simulation simulation("", step);
	FactionId faction = simulation.createFaction("Tower Of Power");
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=dwarf,
		.percentGrown=Percent::create(45),
		.location=blocks.getIndex_i(1, 1, 2),
		.percentHunger=Percent::create(0),
		.percentTired=Percent::create(0),
		.percentThirst=Percent::create(0),
	});
	actors.setFaction(actor, faction);
	CHECK(actors.grow_isGrowing(actor));
	blocks.solid_setNot(blocks.getIndex_i(7, 7, 0));
	blocks.fluid_add(blocks.getIndex_i(7, 7, 0), CollisionVolume::create(100), water);
	items.create({
		.itemType=ItemType::byName("apple"),
		.materialType=MaterialType::byName("fruit"),
		.location=blocks.getIndex_i(1, 5, 2),
		.quantity=Quantity::create(1000),
	});
	CHECK(actors.grow_getEventPercent(actor) == 0);
	CHECK(actors.grow_getPercent(actor) == 45);
	CHECK(actors.grow_isGrowing(actor));
	Step nextPercentIncreaseStep = actors.grow_getEventStep(actor);
	CHECK(nextPercentIncreaseStep <= simulation.m_step + Config::stepsPerDay * 95);
	CHECK(nextPercentIncreaseStep >= simulation.m_step + Config::stepsPerDay * 90);
	simulation.fasterForward(Config::stepsPerHour * 26);
	actors.satisfyNeeds(actor);
	CHECK(actors.grow_isGrowing(actor));
	Percent percentGrowthEventComplete = actors.grow_getEventPercent(actor);
	CHECK(percentGrowthEventComplete == 1);
	actors.grow_setPercent(actor, Percent::create(20));
	CHECK(actors.grow_getPercent(actor) == 20);
	CHECK(actors.grow_getEventPercent(actor) == 0);
	CHECK(actors.grow_isGrowing(actor));
	actors.grow_setPercent(actor, Percent::create(100));
	CHECK(actors.grow_getPercent(actor) == 100);
	CHECK(actors.grow_eventIsPaused(actor));
	CHECK(!actors.grow_isGrowing(actor));
	actors.grow_setPercent(actor, Percent::create(30));
	CHECK(actors.grow_getPercent(actor) == 30);
	CHECK(actors.grow_getEventPercent(actor) == 0);
	CHECK(actors.grow_isGrowing(actor));
}
TEST_CASE("death")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId redDeer = AnimalSpecies::byName("red deer");
	Step step  = DateTime(12,150,1000).toSteps();
	Simulation simulation("", step);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	areaBuilderUtil::setSolidWalls(area, 5, MaterialType::byName("marble"));
	Actors& actors = area.getActors();
	BlockIndex actorLocation = blocks.getIndex_i(1, 1, 2);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=redDeer,
		.percentGrown=Percent::create(45),
		.location=actorLocation,
	});
	// Set low priority station to prevent wandering around while waiting for events.
	Priority stationPriority = Priority::create(1);
	actors.objective_addTaskToStart(actor, std::make_unique<StationObjective>(actorLocation, stationPriority));
	SUBCASE("thirst")
	{
		// Generate objectives, discard eat if it exists.
		actors.drink_setNeedsFluid(actor);
		CHECK(actors.objective_getCurrentName(actor) == "drink");
		CHECK(actors.move_hasPathRequest(actor));
		// Fail to find anything to drink, create path request to area edge.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) == "drink");
		CHECK(actors.move_hasPathRequest(actor));
		// Fail to path to edge, supress need.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(actor) != "drink");
		Step delayTillDeathByDehydration = AnimalSpecies::getStepsTillDieWithoutFluid(redDeer);
		// Delay must be even number or divide by  2 will cause a rounding error.
		float delayMod2 = delayTillDeathByDehydration.get() % 2;
		CHECK(delayMod2 == 0);
		simulation.fasterForward(delayTillDeathByDehydration / 2);
		CHECK(actors.drink_getPercentDead(actor) == 50);
		if(actors.eat_getMassFoodRequested(actor) != 0)
			actors.eat_do(actor, actors.eat_getMassFoodRequested(actor));
		delayTillDeathByDehydration = actors.drink_getStepsTillDead(actor);
		simulation.fasterForward(delayTillDeathByDehydration - 1);
		CHECK(actors.isAlive(actor));
		simulation.doStep();
		CHECK(!actors.isAlive(actor));
		CHECK(actors.getCauseOfDeath(actor) == CauseOfDeath::thirst);
	}
	SUBCASE("hunger")
	{
		BlockIndex pondLocation = blocks.getIndex_i(3, 3, 1);
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, CollisionVolume::create(100), FluidType::byName("water"));
		// Generate objectives, discard drink if it exists.
		CHECK(actors.eat_getHungerEventStep(actor) == AnimalSpecies::getStepsEatFrequency(redDeer) +  step);
		actors.eat_setIsHungry(actor);
		simulation.fasterForward(AnimalSpecies::getStepsTillDieWithoutFood(redDeer) - 1);
		CHECK(actors.isAlive(actor));
		simulation.doStep();
		CHECK(!actors.isAlive(actor));
		CHECK(actors.getCauseOfDeath(actor) == CauseOfDeath::hunger);
	}
}
TEST_CASE("death-temperature")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId redDeer = AnimalSpecies::byName("red deer");
	Step step  = DateTime(12,150,1000).toSteps();
	Simulation simulation("", step);
	Area& area = simulation.m_hasAreas->createArea(6,6,6);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	areaBuilderUtil::setSolidWalls(area, 5, MaterialType::byName("marble"));
	Actors& actors = area.getActors();
	BlockIndex actorLocation = blocks.getIndex_i(1, 1, 2);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=redDeer,
		.percentGrown=Percent::create(45),
		.location=actorLocation,
	});
	// Set low priority station to prevent wandering around while waiting for events.
	Priority stationPriority = Priority::create(1);
	actors.objective_addTaskToStart(actor, std::make_unique<StationObjective>(actorLocation, stationPriority));
	SUBCASE("temperature")
	{
		BlockIndex temperatureSourceLocation = blocks.getIndex_i(3, 4, 2);
		BlockIndex b1 = blocks.getIndex_i(3, 3, 2);
		BlockIndex b2 = blocks.getIndex_i(3, 2, 2);
		BlockIndex b3 = blocks.getIndex_i(3, 1, 2);
		BlockIndex b4 = blocks.getIndex_i(2, 1, 2);
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, TemperatureDelta::create(6000));
		simulation.doStep();
		CHECK(blocks.temperature_get(b1) == blocks.temperature_get(temperatureSourceLocation));
		CHECK(blocks.temperature_get(b2) < blocks.temperature_get(b1));
		CHECK(blocks.temperature_get(b3) < blocks.temperature_get(b2));
		CHECK(blocks.temperature_get(b4) < blocks.temperature_get(b3));
		CHECK(blocks.temperature_get(actorLocation) < blocks.temperature_get(b4));
		CHECK(!actors.temperature_isSafeAtCurrentLocation(actor));
		simulation.fasterForward(AnimalSpecies::getStepsTillDieInUnsafeTemperature(actors.getSpecies(actor)) - 2);
		CHECK(actors.isAlive(actor));
		simulation.doStep();
		CHECK(!actors.isAlive(actor));
		CHECK(actors.getCauseOfDeath(actor) == CauseOfDeath::temperature);
	}
}
