#include "../../lib/doctest.h"
#include "../../src/simulation.h"
#include "../../src/animalSpecies.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/plant.h"
TEST_CASE("basicNeedsSentient")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][2], 50);
	actor.setFaction(&faction);
	REQUIRE(actor.m_canGrow.isGrowing());
	area.m_blocks[5][5][2].m_hasPlant.addPlant(wheatGrass, 100);
	SUBCASE("drink from pond")
	{
		Block& pondLocation = area.m_blocks[3][3][1];
		pondLocation.setNotSolid();
		pondLocation.addFluid(100, water);
		simulation.m_step += dwarf.stepsFluidDrinkFreqency;
		simulation.doStep();
		REQUIRE(!actor.m_canGrow.isGrowing());
		REQUIRE(actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "drink");
		simulation.doStep();
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination->isAdjacentToIncludingCornersAndEdges(pondLocation));
		while(actor.m_location != destination)
			simulation.doStep();
		Step drinkStep = simulation.m_step + Config::stepsToDrink;
		while(simulation.m_step <= drinkStep)
			simulation.doStep();
		REQUIRE(!actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(actor);
		REQUIRE(pondLocation.volumeOfFluidTypeContains(water) == 100 - drinkVolume);
	}
	SUBCASE("drink from bucket")
	{
		Item& bucket = simulation.createItem(ItemType::byName("bucket"), MaterialType::byName("poplar wood"), 50u, 0u, nullptr);
		Block& bucketLocation = area.m_blocks[7][7][2];
		bucket.setLocation(bucketLocation);
		bucket.m_hasCargo.add(water, 10);
		simulation.m_step += dwarf.stepsFluidDrinkFreqency;
		simulation.doStep();
		REQUIRE(!actor.m_canGrow.isGrowing());
		REQUIRE(actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "drink");
		simulation.doStep();
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination == &bucketLocation);
		while(actor.m_location != destination)
			simulation.doStep();
		Step drinkStep = simulation.m_step + Config::stepsToDrink;
		while(simulation.m_step <= drinkStep)
			simulation.doStep();
		REQUIRE(!actor.m_mustDrink.needsFluid());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "drink");
		uint32_t drinkVolume = MustDrink::drinkVolumeFor(actor);
		REQUIRE(bucket.m_hasCargo.getFluidVolume() == 10 - drinkVolume);
	}
	SUBCASE("eat prepared meal")
	{
		Item& meal = simulation.createItem(ItemType::byName("prepared meal"), MaterialType::byName("fruit"), 50u, 0u, nullptr);
		Block& mealLocation = area.m_blocks[5][5][2];
		meal.setLocation(mealLocation);
		simulation.m_step += dwarf.stepsEatFrequency;
		simulation.doStep();
		actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(actor.m_mustEat.needsFood());
		simulation.doStep();
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination == &mealLocation);
		while(actor.m_location != destination)
			simulation.doStep();
		Step eatStep = simulation.m_step + Config::stepsToEat;
		while(simulation.m_step <= eatStep)
			simulation.doStep();
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(mealLocation.m_hasItems.empty());
	}
}
TEST_CASE("basicNeedsNonsentient")
{
	static const PlantSpecies& wheatGrass = PlantSpecies::byName("wheat grass");
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const Faction faction(L"Tower of Power");
	static const AnimalSpecies& redDeer = AnimalSpecies::byName("red deer");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	area.m_hasFarmFields.registerFaction(faction);
	Actor& actor = simulation.createActor(redDeer, area.m_blocks[1][1][2], 50);
	actor.setFaction(&faction);
	REQUIRE(actor.m_canGrow.isGrowing());
	Block& grassLocation = area.m_blocks[5][5][2];
	grassLocation.m_hasPlant.addPlant(wheatGrass, 100);
	Plant& grass = grassLocation.m_hasPlant.get();
	Block& pondLocation = area.m_blocks[3][3][1];
	pondLocation.setNotSolid();
	pondLocation.addFluid(100, water);
	SUBCASE("eat leaves")
	{
		// Generate objectives, discard drink.
		simulation.m_step += redDeer.stepsEatFrequency;
		simulation.doStep();
		actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "eat");
		REQUIRE(actor.m_mustEat.needsFood());
		REQUIRE(grass.getPercentFoliage() == 100);
		// Find grass.
		simulation.doStep();
		Block* destination = actor.m_canMove.getDestination();
		REQUIRE(destination != nullptr);
		REQUIRE(destination == &grassLocation);
		// Go to grass.
		while(actor.m_location != destination)
			simulation.doStep();
		// Eat grass.
		Step eatStep = simulation.m_step + Config::stepsToEat;
		while(simulation.m_step <= eatStep)
			simulation.doStep();
		REQUIRE(!actor.m_mustEat.needsFood());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() != "eat");
		REQUIRE(grass.getPercentFoliage() != 100);
	}
	SUBCASE("sleep outside")
	{
		simulation.m_step += redDeer.stepsSleepFrequency;
		// Generate objectives, discard eat and drink if they exist.
		simulation.doStep();
		actor.m_stamina.spend(1);
		REQUIRE(!actor.m_stamina.isFull());
		actor.m_mustDrink.drink(actor.m_mustDrink.getVolumeFluidRequested());
		actor.m_mustEat.eat(actor.m_mustEat.getMassFoodRequested());
		REQUIRE(actor.m_hasObjectives.getCurrent().name() == "sleep");
		REQUIRE(actor.m_awake);
		// Look for sleeping spot.
		simulation.doStep();
		// No spot found better then current one.
		REQUIRE(actor.m_canMove.getDestination() == nullptr);
		REQUIRE(!actor.m_awake);
		// Wait for wake up.
		Step wakeStep = simulation.m_step + redDeer.stepsSleepDuration;
		while(simulation.m_step != wakeStep)
			simulation.doStep();
		REQUIRE(actor.m_awake);
		REQUIRE(actor.m_mustSleep.hasTiredEvent());
		REQUIRE(actor.m_stamina.isFull());
	}
}
