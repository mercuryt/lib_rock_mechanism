#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/plant.h"
#include "actor.h"
#include "types.h"
TEST_CASE("basicNeedsSentient")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& actor = simulation.m_hasActors->createActor(ActorParamaters{
		.species=dwarf, 
		.percentGrown=50,
		.location=blocks.getIndex({1, 1, 2}), 
		.area=&area,
		.percentHunger=0,
		.percentTired=0,
		.percentThirst=0,
	});
	actor.setFaction(&faction);
	REQUIRE(actor.m_canGrow.isGrowing());
	SUBCASE("drink from pond")
	{
		BlockIndex pondLocation = blocks.getIndex({3, 3, 1});
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, 100, water);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		REQUIRE(!actor.m_canGrow.isGrowing());
		REQUIRE(actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "drink");
		simulation.doStep();
		BlockIndex destination = actor.m_canMove.getDestination();
		REQUIRE(destination != BLOCK_INDEX_MAX);
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(actor, pondLocation);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(actor);
		simulation.doStep(); // Give a step for the fluid removal to take effect.
		REQUIRE(blocks.fluid_volumeOfTypeContains(pondLocation, water) == Config::maxBlockVolume - drinkVolume);
	}
	SUBCASE("drink from bucket")
	{
		Item& bucket = simulation.m_hasItems->createItemNongeneric(ItemType::byName("bucket"), MaterialType::byName("poplar wood"), 50u, 0u, nullptr);
		BlockIndex bucketLocation = blocks.getIndex({7, 7, 2});
		bucket.setLocation(bucketLocation);
		bucket.m_hasCargo.add(water, 10);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		REQUIRE(!actor.m_canGrow.isGrowing());
		REQUIRE(actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "drink");
		simulation.doStep();
		BlockIndex destination = actor.m_canMove.getDestination();
		REQUIRE(destination != BLOCK_INDEX_MAX);
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, bucketLocation));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(actor, bucket);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(actor);
		REQUIRE(bucket.m_hasCargo.getFluidVolume() == 10 - drinkVolume);
	}
	SUBCASE("drink tea")
	{
		//TODO
	}
	SUBCASE("eat prepared meal")
	{
		Item& meal = simulation.m_hasItems->createItemNongeneric(ItemType::byName("prepared meal"), MaterialType::byName("fruit"), 50u, 0u, nullptr);
		BlockIndex mealLocation = blocks.getIndex({5, 5, 2});
		meal.setLocation(mealLocation);
		REQUIRE(actor.m_mustEat.getDesireToEatSomethingAt(mealLocation) == UINT32_MAX);
		simulation.fastForward(dwarf.stepsEatFrequency);
		REQUIRE(actor.m_mustEat.hasObjecive());
		// Discard drink objective if exists.
		if(actor.m_mustDrink.getVolumeFluidRequested() != 0)
			actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(actor.m_mustEat.needsFood());
		simulation.doStep();
		BlockIndex destination = actor.m_canMove.getDestination();
		REQUIRE(destination != BLOCK_INDEX_MAX);
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, mealLocation));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(actor, meal);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(static_cast<EatObjective&>(actor.m_hasObjectives.getCurrent()).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(blocks.item_empty(mealLocation));
	}
	SUBCASE("eat fruit")
	{
		Item& fruit = simulation.m_hasItems->createItemGeneric(ItemType::byName("apple"), MaterialType::byName("fruit"), 50u);
		BlockIndex fruitLocation = blocks.getIndex({6, 5, 2});
		fruit.setLocation(fruitLocation);
		REQUIRE(actor.m_mustEat.getDesireToEatSomethingAt(fruitLocation) == 2);
		simulation.fastForward(dwarf.stepsEatFrequency);
		// Discard drink objective if exists.
		if(actor.m_mustDrink.getVolumeFluidRequested() != 0)
			actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(actor.m_mustEat.needsFood());
		simulation.doStep();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(fruit.isAdjacentTo(actor.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(actor, fruit);
		REQUIRE(actor.isAdjacentTo(fruit));
		REQUIRE(static_cast<EatObjective&>(actor.m_hasObjectives.getCurrent()).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(!blocks.item_empty(fruitLocation));
		REQUIRE(fruit.getQuantity() < 50u);
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
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& actor = simulation.m_hasActors->createActor(ActorParamaters{
		.species=redDeer, 
		.percentGrown=50,
		.location=blocks.getIndex({1, 1, 2}),
		.area=&area,
	});
	REQUIRE(actor.m_canGrow.isGrowing());
	REQUIRE(actor.m_mustDrink.thirstEventExists());
	BlockIndex pondLocation = blocks.getIndex({3, 3, 1});
	blocks.solid_setNot(pondLocation);
	blocks.fluid_add(pondLocation, 100, water);
	SUBCASE("sleep outside at current location")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(redDeer.stepsSleepFrequency);
		// Discard drink objective if exists.
		if(actor.m_mustDrink.getVolumeFluidRequested() != 0)
			actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		actor.m_stamina.spend(1);
		REQUIRE(!actor.m_stamina.isFull());
		if(actor.m_mustEat.getMassFoodRequested() != 0)
			actor.m_mustEat.eat(actor.m_mustEat.getMassFoodRequested());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
		REQUIRE(actor.m_mustSleep.isAwake());
		// Look for sleeping spot.
		simulation.doStep();
		// No spot found better then current one.
		REQUIRE(actor.m_canMove.getDestination() == BLOCK_INDEX_MAX);
		REQUIRE(!actor.m_mustSleep.isAwake());
		// Wait for wake up.
		simulation.fastForward(redDeer.stepsSleepDuration);
		REQUIRE(actor.m_mustSleep.isAwake());
		REQUIRE(actor.m_mustSleep.hasTiredEvent());
		REQUIRE(actor.m_stamina.isFull());
	}
	SUBCASE("eat leaves")
	{
		BlockIndex grassLocation = blocks.getIndex({5, 5, 2});
		blocks.plant_create(grassLocation, wheatGrass, 100);
		Plant& grass = blocks.plant_get(grassLocation);
		// Generate objectives.
		simulation.fastForward(redDeer.stepsEatFrequency);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(actor.m_mustEat.needsFood());
		REQUIRE(grass.getPercentFoliage() == 100);
		// The deer is wandering randomly while we fast forward until it is hungry, it might or might not already be next to the grass.
		if(!actor.isAdjacentTo(grassLocation))
		{
			// Find grass.
			simulation.doStep();
			REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
			BlockIndex destination = actor.m_canMove.getDestination();
			REQUIRE(destination != BLOCK_INDEX_MAX);
			REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(destination, grassLocation));
			// Go to grass.
			simulation.fastForwardUntillActorIsAdjacentToDestination(actor, grassLocation);
		}
		// Eat grass.
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(grass.getPercentFoliage() != 100);
	}
	SUBCASE("sleep at assigned spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 2, 2}), blocks.getIndex({8, 2, 2}), dirt);
		BlockIndex gateway = blocks.getIndex({9, 2, 2});
		BlockIndex spot = blocks.getIndex({5, 5, 2});
		actor.m_mustSleep.setLocation(spot);
		actor.m_mustSleep.tired();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
		// Path to spot.
		simulation.doStep();
		REQUIRE(actor.m_canMove.getDestination() == spot);
		SUBCASE("success")
		{
			simulation.fastForwardUntillActorIsAtDestination(actor, spot);
			// Wait for wake up.
			simulation.fastForward(redDeer.stepsSleepDuration);
			REQUIRE(actor.m_mustSleep.isAwake());
			REQUIRE(actor.m_mustSleep.hasTiredEvent());
		}
		SUBCASE("spot no longer suitable")
		{
			blocks.solid_set(spot, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentTo(actor, spot);
			REQUIRE(actor.m_canMove.getDestination() == BLOCK_INDEX_MAX);
			REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
			REQUIRE(actor.m_mustSleep.getLocation() == BLOCK_INDEX_MAX);
			REQUIRE(actor.m_mustSleep.getObjective()->threadedTaskExists());
		}
		SUBCASE("cannot path to spot")
		{
			blocks.solid_set(gateway, dirt, false);
			simulation.fastForwardUntillActorIsAdjacentTo(actor, gateway);
			REQUIRE(actor.m_canMove.hasThreadedTask());
			// Path to designated spot blocked, try to repath, no path found, clear designated spot.
			simulation.doStep();
			REQUIRE(actor.m_mustSleep.getLocation() == BLOCK_INDEX_MAX);
			REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
			REQUIRE(actor.m_mustSleep.getObjective()->threadedTaskExists());
		}
	}
	SUBCASE("try to find a spot inside")
	{
		//TODO
	}
	SUBCASE("scavenge corpse")
	{
		const AnimalSpecies& blackBear = AnimalSpecies::byName("black bear");
		Actor& bear = simulation.m_hasActors->createActor(ActorParamaters{
			.species=blackBear, 
			.location=blocks.getIndex({5, 1, 2}),
			.area=&area
		});
		Actor& deer = actor;
		uint32_t deerMass = deer.getMass();
		deer.die(CauseOfDeath::thirst);
		REQUIRE(!deer.isAlive());
		simulation.fastForward(blackBear.stepsEatFrequency);
		// Bear is hungry.
		REQUIRE(bear.m_mustEat.getMassFoodRequested() != 0);
		// Bear goes to deer corpse.
		simulation.doStep();
		if(!bear.isAdjacentTo(deer))
			simulation.fastForwardUntillActorIsAdjacentToHasShape(bear, deer);
		// Bear eats.
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!bear.m_mustEat.needsFood());
		REQUIRE(bear.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(deer.getMass() != deerMass);
	}
	SUBCASE("leave location with unsafe temperature")
	{
		REQUIRE(actor.m_canGrow.isGrowing());
		BlockIndex temperatureSourceLocation = blocks.getIndex({1, 1, 3});
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, 200);
		simulation.doStep();
		REQUIRE(!actor.m_needsSafeTemperature.isSafeAtCurrentLocation());
		REQUIRE(blocks.temperature_get(actor.m_location) > actor.m_species.maximumSafeTemperature);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "get to safe temperature");
		REQUIRE(!actor.m_canGrow.isGrowing());
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
	Actor& actor = simulation.m_hasActors->createActor(ActorParamaters{
		.species=redDeer, 
		.percentGrown=50,
		.location=blocks.getIndex({1, 1, 2}), 
		.area=&area,
	});
	SUBCASE("thirst")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(redDeer.stepsFluidDrinkFreqency);
		if(actor.m_mustEat.getMassFoodRequested() != 0)
			actor.m_mustEat.eat(actor.m_mustEat.getMassFoodRequested());
		// Subtract 2 rather then 1 because the event was scheduled on the previous step.
		simulation.fastForward(redDeer.stepsTillDieWithoutFluid - 2);
		REQUIRE(actor.isAlive());
		simulation.doStep();
		REQUIRE(!actor.isAlive());
		REQUIRE(actor.getCauseOfDeath() == CauseOfDeath::thirst);
	}
	SUBCASE("hunger")
	{
		BlockIndex pondLocation = blocks.getIndex({3, 3, 1});
		blocks.solid_setNot(pondLocation);
		blocks.fluid_add(pondLocation, 100, FluidType::byName("water"));
		// Generate objectives, discard drink if it exists.
		REQUIRE(actor.m_mustEat.getHungerEventStep() == redDeer.stepsEatFrequency +  step);
		simulation.fastForward(redDeer.stepsEatFrequency);
		REQUIRE(actor.m_mustEat.getHungerEventStep() == redDeer.stepsEatFrequency + redDeer.stepsTillDieWithoutFood + step);
		simulation.fastForward(redDeer.stepsTillDieWithoutFood - 2);
		REQUIRE(actor.isAlive());
		simulation.doStep();
		REQUIRE(!actor.isAlive());
		REQUIRE(actor.getCauseOfDeath() == CauseOfDeath::hunger);
	}
	SUBCASE("temperature")
	{
		BlockIndex temperatureSourceLocation = blocks.getIndex({5, 5, 2});
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, 60000);
		simulation.doStep();
		REQUIRE(!actor.m_needsSafeTemperature.isSafeAtCurrentLocation());
		simulation.fastForward(actor.m_species.stepsTillDieInUnsafeTemperature - 2);
		REQUIRE(actor.isAlive());
		simulation.doStep();
		REQUIRE(!actor.isAlive());
		REQUIRE(actor.getCauseOfDeath() == CauseOfDeath::temperature);
	}
}
