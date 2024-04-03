#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/plant.h"
#include "actor.h"
TEST_CASE("basicNeedsSentient")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& actor = simulation.createActor(ActorParamaters{
		.species=dwarf, 
		.percentGrown=50,
		.location=&area.getBlock(1, 1, 2), 
		.percentHunger=0,
		.percentTired=0,
		.percentThirst=0,
	});
	actor.setFaction(&faction);
	REQUIRE(actor.m_canGrow.isGrowing());
	SUBCASE("drink from pond")
	{
		Block& pondLocation = area.getBlock(3, 3, 1);
		pondLocation.setNotSolid();
		pondLocation.addFluid(100, water);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		REQUIRE(!actor.m_canGrow.isGrowing());
		REQUIRE(actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "drink");
		simulation.doStep();
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination->isAdjacentToIncludingCornersAndEdges(pondLocation));
		simulation.fastForwardUntillActorIsAdjacentToDestination(actor, pondLocation);
		simulation.fastForward(Config::stepsToDrink);
		REQUIRE(!actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(actor);
		simulation.doStep(); // Give a step for the fluid removal to take effect.
		REQUIRE(pondLocation.volumeOfFluidTypeContains(water) == Config::maxBlockVolume - drinkVolume);
	}
	SUBCASE("drink from bucket")
	{
		Item& bucket = simulation.createItemNongeneric(ItemType::byName("bucket"), MaterialType::byName("poplar wood"), 50u, 0u, nullptr);
		Block& bucketLocation = area.getBlock(7, 7, 2);
		bucket.setLocation(bucketLocation);
		bucket.m_hasCargo.add(water, 10);
		simulation.fastForward(dwarf.stepsFluidDrinkFreqency);
		REQUIRE(!actor.m_canGrow.isGrowing());
		REQUIRE(actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "drink");
		simulation.doStep();
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination->isAdjacentToIncludingCornersAndEdges(bucketLocation));
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
		Item& meal = simulation.createItemNongeneric(ItemType::byName("prepared meal"), MaterialType::byName("fruit"), 50u, 0u, nullptr);
		Block& mealLocation = area.getBlock(5, 5, 2);
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
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination->isAdjacentToIncludingCornersAndEdges(mealLocation));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(actor, meal);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(static_cast<EatObjective&>(actor.m_hasObjectives.getCurrent()).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(mealLocation.m_hasItems.empty());
	}
	SUBCASE("eat fruit")
	{
		Item& fruit = simulation.createItemGeneric(ItemType::byName("apple"), MaterialType::byName("fruit"), 50u);
		Block& fruitLocation = area.getBlock(6, 5, 2);
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
		REQUIRE(fruit.isAdjacentTo(*actor.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(actor, fruit);
		REQUIRE(actor.isAdjacentTo(fruit));
		REQUIRE(static_cast<EatObjective&>(actor.m_hasObjectives.getCurrent()).hasEvent());
		simulation.fastForward(Config::stepsToEat);
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(!fruitLocation.m_hasItems.empty());
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
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& actor = simulation.createActor(redDeer, area.getBlock(1, 1, 2), 50);
	REQUIRE(actor.m_canGrow.isGrowing());
	REQUIRE(actor.m_mustDrink.thirstEventExists());
	Block& pondLocation = area.getBlock(3, 3, 1);
	pondLocation.setNotSolid();
	pondLocation.addFluid(100, water);
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
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
		REQUIRE(!actor.m_mustSleep.isAwake());
		// Wait for wake up.
		simulation.fastForward(redDeer.stepsSleepDuration);
		REQUIRE(actor.m_mustSleep.isAwake());
		REQUIRE(actor.m_mustSleep.hasTiredEvent());
		REQUIRE(actor.m_stamina.isFull());
	}
	SUBCASE("eat leaves")
	{
		Block& grassLocation = area.getBlock(5, 5, 2);
		grassLocation.m_hasPlant.createPlant(wheatGrass, 100);
		Plant& grass = grassLocation.m_hasPlant.get();
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
			Block* destination = actor.m_canMove.getDestination();
			REQUIRE(destination != nullptr);
			REQUIRE(destination->isAdjacentToIncludingCornersAndEdges(grassLocation));
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
		areaBuilderUtil::setSolidWall(area.getBlock(0, 2, 2), area.getBlock(8, 2, 2), dirt);
		Block& gateway = area.getBlock(9, 2, 2);
		Block& spot = area.getBlock(5, 5, 2);
		actor.m_mustSleep.setLocation(spot);
		actor.m_mustSleep.tired();
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
		// Path to spot.
		simulation.doStep();
		REQUIRE(actor.m_canMove.getDestination() == &spot);
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
			spot.setSolid(dirt);
			simulation.fastForwardUntillActorIsAdjacentTo(actor, spot);
			REQUIRE(actor.m_canMove.getDestination() == nullptr);
			REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
			REQUIRE(actor.m_mustSleep.getLocation() == nullptr);
			REQUIRE(actor.m_mustSleep.getObjective()->threadedTaskExists());
		}
		SUBCASE("cannot path to spot")
		{
			gateway.setSolid(dirt);
			simulation.fastForwardUntillActorIsAdjacentTo(actor, gateway);
			REQUIRE(actor.m_canMove.hasThreadedTask());
			// Path to designated spot blocked, try to repath, no path found, clear designated spot.
			simulation.doStep();
			REQUIRE(actor.m_mustSleep.getLocation() == nullptr);
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
		Actor& bear = simulation.createActor(blackBear, area.getBlock(5, 1, 2));
		Actor& deer = actor;
		uint32_t deerMass = deer.getMass();
		deer.die(CauseOfDeath::thirst);
		REQUIRE(!deer.m_alive);
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
		Block& temperatureSourceLocation = area.getBlock(1, 1, 3);
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, 200);
		simulation.doStep();
		REQUIRE(!actor.m_needsSafeTemperature.isSafeAtCurrentLocation());
		REQUIRE(actor.m_location->m_blockHasTemperature.get() > actor.m_species.maximumSafeTemperature);
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
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	areaBuilderUtil::setSolidWalls(area, 5, MaterialType::byName("marble"));
	Actor& actor = simulation.createActor(redDeer, area.getBlock(1, 1, 2), 50);
	SUBCASE("thirst")
	{
		// Generate objectives, discard eat if it exists.
		simulation.fastForward(redDeer.stepsFluidDrinkFreqency);
		if(actor.m_mustEat.getMassFoodRequested() != 0)
			actor.m_mustEat.eat(actor.m_mustEat.getMassFoodRequested());
		// Subtract 2 rather then 1 because the event was scheduled on the previous step.
		simulation.fastForward(redDeer.stepsTillDieWithoutFluid - 2);
		REQUIRE(actor.m_alive);
		simulation.doStep();
		REQUIRE(!actor.m_alive);
		REQUIRE(actor.m_causeOfDeath == CauseOfDeath::thirst);
	}
	SUBCASE("hunger")
	{
		Block& pondLocation = area.getBlock(3, 3, 1);
		pondLocation.setNotSolid();
		pondLocation.addFluid(100, FluidType::byName("water"));
		// Generate objectives, discard drink if it exists.
		REQUIRE(actor.m_mustEat.getHungerEventStep() == redDeer.stepsEatFrequency +  step);
		simulation.fastForward(redDeer.stepsEatFrequency);
		REQUIRE(actor.m_mustEat.getHungerEventStep() == redDeer.stepsEatFrequency + redDeer.stepsTillDieWithoutFood + step);
		simulation.fastForward(redDeer.stepsTillDieWithoutFood - 2);
		REQUIRE(actor.m_alive);
		simulation.doStep();
		REQUIRE(!actor.m_alive);
		REQUIRE(actor.m_causeOfDeath == CauseOfDeath::hunger);
	}
	SUBCASE("temperature")
	{
		Block& temperatureSourceLocation = area.getBlock(5, 5, 2);
		area.m_hasTemperature.addTemperatureSource(temperatureSourceLocation, 60000);
		simulation.doStep();
		REQUIRE(!actor.m_needsSafeTemperature.isSafeAtCurrentLocation());
		simulation.fastForward(actor.m_species.stepsTillDieInUnsafeTemperature - 2);
		REQUIRE(actor.m_alive);
		simulation.doStep();
		REQUIRE(!actor.m_alive);
		REQUIRE(actor.m_causeOfDeath == CauseOfDeath::temperature);
	}
}
