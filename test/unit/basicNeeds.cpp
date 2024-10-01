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
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/itemType.h"
#include "../../engine/objectives/eat.h"
#include "types.h"
TEST_CASE("basicNeedsSentient")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation;
	FactionId faction = simulation.createFaction(L"Tower Of Power");
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
	REQUIRE(actors.grow_isGrowing(actor));
	SUBCASE("drink from pond")
	{
		BlockIndex pondLocation = blocks.getIndex_i(3, 3, 1);
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, CollisionVolume::create(100), water);
		simulation.fastForward(AnimalSpecies::getStepsFluidDrinkFrequency(dwarf));
		REQUIRE(!actors.grow_isGrowing(actor));
		REQUIRE(actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "drink");
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		REQUIRE(destination.exists());
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, pondLocation);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "drink");
		CollisionVolume drinkVolume = MustDrink::drinkVolumeFor(area, actor);
		simulation.doStep(); // Give a step for the fluid removal to take effect.
		REQUIRE(blocks.fluid_volumeOfTypeContains(pondLocation, water) == Config::maxBlockVolume - drinkVolume);
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
		items.cargo_addFluid(bucket, water, CollisionVolume::create(10));
		simulation.fastForward(AnimalSpecies::getStepsFluidDrinkFrequency(dwarf));
		REQUIRE(!actors.grow_isGrowing(actor));
		REQUIRE(actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "drink");
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		REQUIRE(destination.exists());
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, bucketLocation));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, bucket);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "drink");
		CollisionVolume drinkVolume = MustDrink::drinkVolumeFor(area, actor);
		REQUIRE(items.cargo_getFluidVolume(bucket) == CollisionVolume::create(10) - drinkVolume);
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
		REQUIRE(actors.eat_getDesireToEatSomethingAt(actor, mealLocation) == maxRankedEatDesire);
		simulation.fastForward(AnimalSpecies::getStepsEatFrequency(dwarf));
		REQUIRE(actors.eat_hasObjective(actor));
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.eat_isHungry(actor));
		simulation.doStep();
		REQUIRE(!actors.move_hasPathRequest(actor));
		BlockIndex destination = actors.move_getDestination(actor);
		REQUIRE(destination.exists());
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, mealLocation));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, meal);
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.objective_getCurrent<EatObjective>(actor).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actors.eat_isHungry(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "eat");
		REQUIRE(blocks.item_empty(mealLocation));
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
		REQUIRE(actors.eat_getDesireToEatSomethingAt(actor, fruitLocation) == 2);
		simulation.fastForward(AnimalSpecies::getStepsEatFrequency(dwarf));
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		REQUIRE(actors.eat_getPercentStarved(actor) == 0);
		REQUIRE(actors.eat_getMinimumAcceptableDesire(actor) == 3);
		simulation.fastForwardUntillPredicate([&](){ return actors.eat_getMinimumAcceptableDesire(actor) == 2; }, Config::minutesPerHour * 6);
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		const EatObjective& objective = actors.objective_getCurrent<EatObjective>(actor);
		REQUIRE(actors.eat_canEatItem(actor, fruit));
		REQUIRE(objective.canEatAt(area, fruitLocation, actor));
		REQUIRE(!objective.hasLocation());
		simulation.doStep();
		// Because we aren't finding a max desirable target we can't use a path generated from the first step of searching, we need a second step to find a path to the selected candidate.
		REQUIRE(actors.move_hasPathRequest(actor));
		REQUIRE(objective.hasLocation());
		simulation.doStep();
		REQUIRE(!actors.move_hasPathRequest(actor));
		REQUIRE(actors.eat_isHungry(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.move_getDestination(actor).exists());
		REQUIRE(items.isAdjacentToLocation(fruit, actors.move_getDestination(actor)));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, fruit);
		REQUIRE(actors.objective_getCurrent<EatObjective>(actor).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actors.eat_isHungry(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "eat");
		REQUIRE(!blocks.item_empty(fruitLocation));
		REQUIRE(items.getQuantity(fruit) < 50u);
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
	REQUIRE(actors.grow_isGrowing(actor));
	REQUIRE(actors.drink_hasThristEvent(actor));
	BlockIndex pondLocation = blocks.getIndex_i(3, 3, 1);
	blocks.solid_setNot(pondLocation);
	blocks.fluid_add(pondLocation, CollisionVolume::create(100), water);
	SUBCASE("sleep outside at current location")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(AnimalSpecies::getStepsSleepFrequency(redDeer));
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		actors.stamina_spend(actor, Stamina::create(1));
		REQUIRE(!actors.stamina_isFull(actor));
		if(actors.eat_getMassFoodRequested(actor) != 0)
			actors.eat_do(actor, actors.eat_getMassFoodRequested(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "sleep");
		REQUIRE(actors.sleep_isAwake(actor));
		// Look for sleeping spot.
		simulation.doStep();
		// No spot found better then current one.
		REQUIRE(actors.move_getDestination(actor).empty());
		REQUIRE(!actors.sleep_isAwake(actor));
		// Wait for wake up.
		simulation.fastForward(AnimalSpecies::getStepsSleepDuration(redDeer));
		REQUIRE(actors.sleep_isAwake(actor));
		REQUIRE(actors.sleep_hasTiredEvent(actor));
		REQUIRE(actors.stamina_isFull(actor));
	}
	SUBCASE("eat leaves")
	{
		BlockIndex grassLocation = blocks.getIndex_i(5, 5, 2);
		blocks.plant_create(grassLocation, wheatGrass, Percent::create(100));
		PlantIndex grass = blocks.plant_get(grassLocation);
		// Generate objectives.
		simulation.fastForward(AnimalSpecies::getStepsEatFrequency(redDeer));
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.eat_isHungry(actor));
		REQUIRE(plants.getPercentFoliage(grass) == 100);
		// The deer is wandering randomly while we fast forward until it is hungry, it might or might not already be next to the grass.
		if(!actors.isAdjacentToLocation(actor, grassLocation))
		{
			// Find grass.
			REQUIRE(actors.move_hasPathRequest(actor));
			simulation.doStep();
			REQUIRE(!actors.move_hasPathRequest(actor));
			REQUIRE(actors.objective_getCurrentName(actor) == "eat");
			BlockIndex destination = actors.move_getDestination(actor);
			REQUIRE(destination.exists());
			REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, grassLocation));
			// Go to grass.
			simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, grassLocation);
		}
		// Eat grass.
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actors.eat_isHungry(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "eat");
		REQUIRE(plants.getPercentFoliage(grass) != 100);
	}
	SUBCASE("sleep at assigned spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 2, 2), blocks.getIndex_i(8, 2, 2), dirt);
		BlockIndex gateway = blocks.getIndex_i(9, 2, 2);
		BlockIndex spot = blocks.getIndex_i(5, 5, 2);
		actors.sleep_setSpot(actor, spot);
		actors.sleep_makeTired(actor);
		REQUIRE(actors.objective_getCurrentName(actor) == "sleep");
		// Path to spot.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(actor) == spot);
		SUBCASE("success")
		{
			simulation.fastForwardUntillActorIsAtDestination(area, actor, spot);
			// Wait for wake up.
			simulation.fastForward(AnimalSpecies::getStepsSleepDuration(redDeer));
			REQUIRE(actors.sleep_isAwake(actor));
			REQUIRE(actors.sleep_hasTiredEvent(actor));
		}
		SUBCASE("spot no longer suitable")
		{
			blocks.solid_set(spot, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, spot);
			REQUIRE(actors.move_getDestination(actor).empty());
			REQUIRE(actors.objective_getCurrentName(actor) == "sleep");
			REQUIRE(actors.sleep_getSpot(actor).empty());
			REQUIRE(actors.move_hasPathRequest(actor));
		}
		SUBCASE("cannot path to spot")
		{
			blocks.solid_set(gateway, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
			REQUIRE(actors.move_hasPathRequest(actor));
			// Path to designated spot blocked, try to repath, no path found, clear designated spot.
			simulation.doStep();
			REQUIRE(actors.sleep_getSpot(actor).empty());
			REQUIRE(actors.objective_getCurrentName(actor) == "sleep");
			REQUIRE(actors.move_hasPathRequest(actor));
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
		REQUIRE(!actors.isAlive(deer));
		REQUIRE(actors.eat_getDesireToEatSomethingAt(bear, actors.getLocation(deer)) != 0);
		simulation.fastForward(AnimalSpecies::getStepsEatFrequency(blackBear));
		// Bear is hungry.
		REQUIRE(actors.eat_getMassFoodRequested(bear) != 0);
		REQUIRE(actors.objective_getCurrentName(bear) == "eat");
		REQUIRE(actors.move_hasPathRequest(bear));
		FindPathResult result = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(bear)).findPathTo(actors.getLocation(bear), actors.getShape(bear), actors.getFacing(bear), actors.getLocation(deer));
		REQUIRE(!result.path.empty());
		EatObjective& objective = actors.objective_getCurrent<EatObjective>(bear);
		REQUIRE(!objective.hasLocation());
		// Bear goes to deer corpse.
		simulation.doStep();
		REQUIRE(objective.hasLocation());
		if(!actors.isAdjacentToActor(bear, deer))
			simulation.fastForwardUntillActorIsAdjacentToActor(area, bear, deer);
		// Bear eats.
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actors.eat_isHungry(bear));
		REQUIRE(actors.objective_getCurrentName(bear) != "eat");
		REQUIRE(actors.getMass(deer) != deerMass);
	}
	SUBCASE("leave location with unsafe temperature")
	{
		REQUIRE(actors.grow_isGrowing(actor));
		BlockIndex temperatureSourceLocation = blocks.getIndex_i(1, 1, 3);
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, TemperatureDelta::create(200));
		simulation.doStep();
		REQUIRE(!actors.temperature_isSafeAtCurrentLocation(actor));
		REQUIRE(blocks.temperature_get(actors.getLocation(actor)) > AnimalSpecies::getMaximumSafeTemperature(actors.getSpecies(actor)));
		REQUIRE(actors.objective_getCurrentName(actor) == "get to safe temperature");
		REQUIRE(!actors.grow_isGrowing(actor));
	}
}
TEST_CASE("death")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId redDeer = AnimalSpecies::byName("red deer");
	Step step  = DateTime(12,150,1000).toSteps();
	Simulation simulation(L"", step);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	areaBuilderUtil::setSolidWalls(area, 5, MaterialType::byName("marble"));
	Actors& actors = area.getActors();
	ActorIndex actor = actors.create(ActorParamaters{
		.species=redDeer, 
		.percentGrown=Percent::create(50),
		.location=blocks.getIndex_i(1, 1, 2), 
	});
	SUBCASE("thirst")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(AnimalSpecies::getStepsFluidDrinkFrequency(redDeer));
		if(actors.eat_getMassFoodRequested(actor) != 0)
			actors.eat_do(actor, actors.eat_getMassFoodRequested(actor));
		// Subtract 2 rather then 1 because the event was scheduled on the previous step.
		simulation.fastForward(AnimalSpecies::getStepsTillDieWithoutFluid(redDeer) - 2);
		REQUIRE(actors.isAlive(actor));
		simulation.doStep();
		REQUIRE(!actors.isAlive(actor));
		REQUIRE(actors.getCauseOfDeath(actor) == CauseOfDeath::thirst);
	}
	SUBCASE("hunger")
	{
		BlockIndex pondLocation = blocks.getIndex_i(3, 3, 1);
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, CollisionVolume::create(100), FluidType::byName("water"));
		// Generate objectives, discard drink if it exists.
		REQUIRE(actors.eat_getHungerEventStep(actor) == AnimalSpecies::getStepsEatFrequency(redDeer) +  step);
		simulation.fastForward(AnimalSpecies::getStepsEatFrequency(redDeer));
		REQUIRE(actors.eat_getHungerEventStep(actor) == AnimalSpecies::getStepsEatFrequency(redDeer) + AnimalSpecies::getStepsTillDieWithoutFood(redDeer) + step);
		simulation.fastForward(AnimalSpecies::getStepsTillDieWithoutFood(redDeer) - 2);
		REQUIRE(actors.isAlive(actor));
		simulation.doStep();
		REQUIRE(!actors.isAlive(actor));
		REQUIRE(actors.getCauseOfDeath(actor) == CauseOfDeath::hunger);
	}
	SUBCASE("temperature")
	{
		BlockIndex temperatureSourceLocation = blocks.getIndex_i(5, 5, 2);
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, TemperatureDelta::create(60000));
		simulation.doStep();
		REQUIRE(!actors.temperature_isSafeAtCurrentLocation(actor));
		simulation.fastForward(AnimalSpecies::getStepsTillDieInUnsafeTemperature(actors.getSpecies(actor)) - 2);
		REQUIRE(actors.isAlive(actor));
		simulation.doStep();
		REQUIRE(!actors.isAlive(actor));
		REQUIRE(actors.getCauseOfDeath(actor) == CauseOfDeath::temperature);
	}
	SUBCASE("growth")
	{
		blocks.solid_setNot(blocks.getIndex_i(7, 7, 0));
		blocks.fluid_add(blocks.getIndex_i(7, 7, 0), CollisionVolume::create(100), FluidType::byName("water"));
		Items& items = area.getItems();
		items.create({
			.itemType=ItemType::byName("apple"),
			.materialType=MaterialType::byName("fruit"),
			.location=blocks.getIndex_i(1, 5, 2), 
			.quantity=Quantity::create(1000),
		});
		REQUIRE(actors.grow_getEventPercent(actor) == 0);
		REQUIRE(actors.grow_getPercent(actor) == 45);
		REQUIRE(actors.grow_isGrowing(actor));
		Step nextPercentIncreaseStep = actors.grow_getEventStep(actor);
		REQUIRE(nextPercentIncreaseStep <= simulation.m_step + Config::stepsPerDay * 95);
		REQUIRE(nextPercentIncreaseStep >= simulation.m_step + Config::stepsPerDay * 90);
		simulation.fastForward(Config::stepsPerDay);
		REQUIRE(actors.grow_getPercent(actor) == 45);
		REQUIRE(!actors.grow_isGrowing(actor));
		simulation.fastForward(Config::stepsPerHour * 2);
		REQUIRE(actors.grow_isGrowing(actor));
		REQUIRE(actors.grow_getEventPercent(actor) == 1);
		REQUIRE(actors.grow_isGrowing(actor));
		//TODO: This is too slow for unit tests, move to integration and replace with a version which sets event percent complete explicitly.
		/*
		simulation.fastForward(Config::stepsPerDay * 5);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 6);
		simulation.fastForward((Config::stepsPerDay * 50));
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 60);
		simulation.fastForward((Config::stepsPerDay * 36));
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 99);
		simulation.fastForwardUntillPredicate([&]{ return dwarf1.m_canGrow.growthPercent() != 45; }, 60 * 24 * 40);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 46);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 0);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
		*/
		actors.grow_setPercent(actor, Percent::create(20));
		REQUIRE(actors.grow_getPercent(actor) == 20);
		REQUIRE(actors.grow_getEventPercent(actor) == 0);
		REQUIRE(actors.grow_isGrowing(actor));
		actors.grow_setPercent(actor, Percent::create(100));
		REQUIRE(actors.grow_getPercent(actor) == 100);
		REQUIRE(actors.grow_eventIsPaused(actor));
		REQUIRE(!actors.grow_isGrowing(actor));
		actors.grow_setPercent(actor, Percent::create(30));
		REQUIRE(actors.grow_getPercent(actor) == 30);
		REQUIRE(actors.grow_getEventPercent(actor) == 0);
		REQUIRE(actors.grow_isGrowing(actor));
	}
}
