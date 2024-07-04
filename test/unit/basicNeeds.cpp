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
TEST_CASE("basicNeedsSentient")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=dwarf, 
		.percentGrown=50,
		.location=blocks.getIndex({1, 1, 2}), 
		.percentHunger=0,
		.percentTired=0,
		.percentThirst=0,
	});
	actors.setFaction(actor, &faction);
	REQUIRE(actors.grow_isGrowing(actor));
	SUBCASE("drink from pond")
	{
		BlockIndex pondLocation = blocks.getIndex({3, 3, 1});
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, 100, water);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		REQUIRE(!actors.grow_isGrowing(actor));
		REQUIRE(actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "drink");
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		REQUIRE(destination != BLOCK_INDEX_MAX);
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, actor, pondLocation);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(area, actor);
		simulation.doStep(); // Give a step for the fluid removal to take effect.
		REQUIRE(blocks.fluid_volumeOfTypeContains(pondLocation, water) == Config::maxBlockVolume - drinkVolume);
	}
	SUBCASE("drink from bucket")
	{
		BlockIndex bucketLocation = blocks.getIndex({7, 7, 2});
		ItemIndex bucket = items.create({
			.itemType=ItemType::byName("bucket"),
			.materialType=MaterialType::byName("poplar wood"),
			.location=bucketLocation,
			.quality=50,
			.percentWear=0,
		});
		items.cargo_addFluid(bucket, water, 10);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		REQUIRE(!actors.grow_isGrowing(actor));
		REQUIRE(actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "drink");
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		REQUIRE(destination != BLOCK_INDEX_MAX);
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, bucketLocation));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, bucket);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actors.drink_isThirsty(actor));
		REQUIRE(actors.objective_getCurrentName(actor) != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(area, actor);
		REQUIRE(items.cargo_getFluidVolume(bucket) == 10 - drinkVolume);
	}
	SUBCASE("drink tea")
	{
		//TODO
	}
	SUBCASE("eat prepared meal")
	{
		BlockIndex mealLocation = blocks.getIndex({5, 5, 2});
		ItemIndex meal = items.create({
			.itemType=ItemType::byName("prepared meal"),
			.materialType=MaterialType::byName("fruit"),
			.location=mealLocation,
			.quality=50,
			.percentWear=0,
		});
		REQUIRE(actors.eat_getDesireToEatSomethingAt(actor, mealLocation) == UINT32_MAX);
		simulation.fastForward(dwarf.stepsEatFrequency);
		REQUIRE(actors.eat_hasObjective(actor));
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.eat_isHungry(actor));
		simulation.doStep();
		BlockIndex destination = actors.move_getDestination(actor);
		REQUIRE(destination != BLOCK_INDEX_MAX);
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
		BlockIndex fruitLocation = blocks.getIndex(6, 5, 2);
		ItemIndex fruit = items.create({
			.itemType=ItemType::byName("apple"),
			.materialType=MaterialType::byName("fruit"),
			.location=fruitLocation,
			.quantity=50,
		});
		REQUIRE(actors.eat_getDesireToEatSomethingAt(actor, fruitLocation) == 2);
		simulation.fastForward(dwarf.stepsEatFrequency);
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.eat_isHungry(actor));
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(items.isAdjacentToLocation(fruit, actors.move_getDestination(actor)));
		simulation.fastForwardUntillActorIsAdjacentToItem(area, actor, fruit);
		REQUIRE(actors.isAdjacentToItem(actor, fruit));
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
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const AnimalSpecies& redDeer = AnimalSpecies::byName("red deer");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	ActorIndex actor = actors.create(ActorParamaters{
		.species=redDeer, 
		.percentGrown=50,
		.location=blocks.getIndex({1, 1, 2}),
	});
	REQUIRE(actors.grow_isGrowing(actor));
	REQUIRE(actors.drink_thirstEventExists(actor));
	BlockIndex pondLocation = blocks.getIndex({3, 3, 1});
	blocks.solid_setNot(pondLocation);
	blocks.fluid_add(pondLocation, 100, water);
	SUBCASE("sleep outside at current location")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(redDeer.stepsSleepFrequency);
		// Discard drink objective if exists.
		if(actors.drink_getVolumeOfFluidRequested(actor) != 0)
			actors.drink_do(actor, actors.drink_getVolumeOfFluidRequested(actor));
		actors.stamina_spend(actor, 1);
		REQUIRE(!actors.stamina_isFull(actor));
		if(actors.eat_getMassFoodRequested(actor) != 0)
			actors.eat_do(actor, actors.eat_getMassFoodRequested(actor));
		REQUIRE(actors.objective_getCurrentName(actor) == "sleep");
		REQUIRE(actors.sleep_isAwake(actor));
		// Look for sleeping spot.
		simulation.doStep();
		// No spot found better then current one.
		REQUIRE(actors.move_getDestination(actor) == BLOCK_INDEX_MAX);
		REQUIRE(!actors.sleep_isAwake(actor));
		// Wait for wake up.
		simulation.fastForward(redDeer.stepsSleepDuration);
		REQUIRE(actors.sleep_isAwake(actor));
		REQUIRE(actors.sleep_hasTiredEvent(actor));
		REQUIRE(actors.stamina_isFull(actor));
	}
	SUBCASE("eat leaves")
	{
		BlockIndex grassLocation = blocks.getIndex(5, 5, 2);
		blocks.plant_create(grassLocation, wheatGrass, 100);
		PlantIndex grass = blocks.plant_get(grassLocation);
		// Generate objectives.
		simulation.fastForward(redDeer.stepsEatFrequency);
		REQUIRE(actors.objective_getCurrentName(actor) == "eat");
		REQUIRE(actors.eat_isHungry(actor));
		REQUIRE(plants.getPercentFoliage(grass) == 100);
		// The deer is wandering randomly while we fast forward until it is hungry, it might or might not already be next to the grass.
		if(!actors.isAdjacentToLocation(actor, grassLocation))
		{
			// Find grass.
			simulation.doStep();
			REQUIRE(actors.objective_getCurrentName(actor) == "eat");
			BlockIndex destination = actors.move_getDestination(actor);
			REQUIRE(destination != BLOCK_INDEX_MAX);
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
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 2, 2}), blocks.getIndex({8, 2, 2}), dirt);
		BlockIndex gateway = blocks.getIndex({9, 2, 2});
		BlockIndex spot = blocks.getIndex({5, 5, 2});
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
			simulation.fastForward(redDeer.stepsSleepDuration);
			REQUIRE(actors.sleep_isAwake(actor));
			REQUIRE(actors.sleep_hasTiredEvent(actor));
		}
		SUBCASE("spot no longer suitable")
		{
			blocks.solid_set(spot, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, spot);
			REQUIRE(actors.move_getDestination(actor) == BLOCK_INDEX_MAX);
			REQUIRE(actors.objective_getCurrentName(actor) == "sleep");
			REQUIRE(actors.sleep_getSpot(actor) == BLOCK_INDEX_MAX);
			REQUIRE(actors.move_hasPathRequest(actor));
		}
		SUBCASE("cannot path to spot")
		{
			blocks.solid_set(gateway, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, actor, gateway);
			REQUIRE(actors.move_hasPathRequest(actor));
			// Path to designated spot blocked, try to repath, no path found, clear designated spot.
			simulation.doStep();
			REQUIRE(actors.sleep_getSpot(actor) == BLOCK_INDEX_MAX);
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
		const AnimalSpecies& blackBear = AnimalSpecies::byName("black bear");
		ActorIndex bear = actors.create(ActorParamaters{
			.species=blackBear, 
			.location=blocks.getIndex({5, 1, 2}),
		});
		ActorIndex deer = actor;
		uint32_t deerMass = actors.getMass(deer);
		actors.die(deer, CauseOfDeath::thirst);
		REQUIRE(!actors.isAlive(deer));
		REQUIRE(actors.eat_getDesireToEatSomethingAt(bear, actors.getLocation(deer)));
		simulation.fastForward(blackBear.stepsEatFrequency);
		// Bear is hungry.
		REQUIRE(actors.eat_getMassFoodRequested(bear) != 0);
		REQUIRE(actors.objective_getCurrentName(bear) == "eat");
		REQUIRE(actors.move_hasPathRequest(bear));
		FindPathResult result = area.m_hasTerrainFacades.at(actors.getMoveType(bear)).findPathTo(actors.getLocation(deer), actors.getShape(bear), actors.getFacing(bear), actors.getLocation(deer));
		REQUIRE(!result.path.empty());
		// Bear goes to deer corpse.
		simulation.doStep();
		if(!actors.isAdjacentToActor(bear, deer))
		{
			REQUIRE(!actors.move_getPath(bear).empty());
			simulation.fastForwardUntillActorIsAdjacentToActor(area, bear, deer);
		}
		// Bear eats.
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actors.eat_isHungry(bear));
		REQUIRE(actors.objective_getCurrentName(bear) != "eat");
		REQUIRE(actors.getMass(deer) != deerMass);
	}
	SUBCASE("leave location with unsafe temperature")
	{
		REQUIRE(actors.grow_isGrowing(actor));
		BlockIndex temperatureSourceLocation = blocks.getIndex({1, 1, 3});
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, 200);
		simulation.doStep();
		REQUIRE(!actors.temperature_isSafeAtCurrentLocation(actor));
		REQUIRE(blocks.temperature_get(actors.getLocation(actor)) > actors.getSpecies(actor).maximumSafeTemperature);
		REQUIRE(actors.objective_getCurrentName(actor) == "get to safe temperature");
		REQUIRE(!actors.grow_isGrowing(actor));
	}
}
TEST_CASE("death")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const AnimalSpecies& redDeer = AnimalSpecies::byName("red deer");
	Step step  = DateTime(12,150,1000).toSteps();
	Simulation simulation(L"", step);
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	areaBuilderUtil::setSolidWalls(area, 5, MaterialType::byName("marble"));
	Actors& actors = area.getActors();
	ActorIndex actor = actors.create(ActorParamaters{
		.species=redDeer, 
		.percentGrown=50,
		.location=blocks.getIndex({1, 1, 2}), 
	});
	SUBCASE("thirst")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(redDeer.stepsFluidDrinkFreqency);
		if(actors.eat_getMassFoodRequested(actor) != 0)
			actors.eat_do(actor, actors.eat_getMassFoodRequested(actor));
		// Subtract 2 rather then 1 because the event was scheduled on the previous step.
		simulation.fastForward(redDeer.stepsTillDieWithoutFluid - 2);
		REQUIRE(actors.isAlive(actor));
		simulation.doStep();
		REQUIRE(!actors.isAlive(actor));
		REQUIRE(actors.getCauseOfDeath(actor) == CauseOfDeath::thirst);
	}
	SUBCASE("hunger")
	{
		BlockIndex pondLocation = blocks.getIndex({3, 3, 1});
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, 100, FluidType::byName("water"));
		// Generate objectives, discard drink if it exists.
		REQUIRE(actors.eat_getHungerEventStep(actor) == redDeer.stepsEatFrequency +  step);
		simulation.fastForward(redDeer.stepsEatFrequency);
		REQUIRE(actors.eat_getHungerEventStep(actor) == redDeer.stepsEatFrequency + redDeer.stepsTillDieWithoutFood + step);
		simulation.fastForward(redDeer.stepsTillDieWithoutFood - 2);
		REQUIRE(actors.isAlive(actor));
		simulation.doStep();
		REQUIRE(!actors.isAlive(actor));
		REQUIRE(actors.getCauseOfDeath(actor) == CauseOfDeath::hunger);
	}
	SUBCASE("temperature")
	{
		BlockIndex temperatureSourceLocation = blocks.getIndex({5, 5, 2});
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, 60000);
		simulation.doStep();
		REQUIRE(!actors.temperature_isSafeAtCurrentLocation(actor));
		simulation.fastForward(actors.getSpecies(actor).stepsTillDieInUnsafeTemperature - 2);
		REQUIRE(actors.isAlive(actor));
		simulation.doStep();
		REQUIRE(!actors.isAlive(actor));
		REQUIRE(actors.getCauseOfDeath(actor) == CauseOfDeath::temperature);
	}
}
