#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/project.h"
#include "../../engine/haul.h"
#include "../../engine/targetedHaul.h"
TEST_CASE("haul")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& gold = MaterialType::byName("gold");
	static const MaterialType& lead = MaterialType::byName("lead");
	static const MaterialType& poplarWood = MaterialType::byName("poplar wood");
	static const MaterialType& peridotite = MaterialType::byName("peridotite");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& donkey = AnimalSpecies::byName("jackstock donkey");
	static const ItemType& chunk = ItemType::byName("chunk");
	static const ItemType& boulder = ItemType::byName("boulder");
	static const ItemType& cart = ItemType::byName("cart");
	static const ItemType& panniers = ItemType::byName("panniers");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
	Faction faction(L"tower of power");
	dwarf1.setFaction(&faction);
	REQUIRE(!dwarf1.m_canPickup.exists());
	SUBCASE("canPickup")
	{
		Block& chunkLocation = area.getBlock(1, 2, 2);
		Item& chunk1 = simulation.createItemGeneric(chunk, marble, 1);
		chunk1.setLocation(chunkLocation);
		Item& chunk2 = simulation.createItemGeneric(chunk, gold, 1);
		chunk2.setLocation(area.getBlock(6, 2, 2));
		REQUIRE(dwarf1.m_canPickup.canPickupAny(chunk1));
		REQUIRE(!dwarf1.m_canPickup.canPickupAny(chunk2));
		dwarf1.m_canPickup.pickUp(chunk1, 1);
		Block& destination = area.getBlock(5, 5, 2);
		dwarf1.m_canMove.setDestination(destination);
		simulation.doStep();
		REQUIRE(!dwarf1.m_canMove.getPath().empty());
		while(dwarf1.m_location != &destination)
			simulation.doStep();
		dwarf1.m_canPickup.putDown(*dwarf1.m_location);
		REQUIRE(destination.m_hasItems.getCount(chunk, marble) == 1);
	}
	SUBCASE("has haul tools")
	{
		Block& cartLocation = area.getBlock(1, 2, 2);
		Item& cart1 = simulation.createItemNongeneric(cart, poplarWood, 3u, 0);
		Item& chunk1 = simulation.createItemGeneric(chunk, gold, 1u);
		cart1.setLocation(cartLocation);
		area.m_hasHaulTools.registerHaulTool(cart1);
		REQUIRE(area.m_hasHaulTools.hasToolToHaul(faction, chunk1));
	}
	SUBCASE("individual haul strategy")
	{
		Block& destination = area.getBlock(5, 5, 2);
		Block& chunkLocation = area.getBlock(1, 5, 2);
		Item& chunk1 = simulation.createItemGeneric(chunk, marble, 1u);
		chunk1.setLocation(chunkLocation);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(std::vector<Actor*>({&dwarf1}), chunk1, destination);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "haul");
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// One step to run the create subproject threaded task and set the strategy.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Another step to find the path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, chunkLocation);
		REQUIRE(dwarf1.m_canPickup.exists());
		REQUIRE(dwarf1.m_canPickup.getItem() == chunk1);
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(destination.m_hasItems.getCount(chunk, marble) == 1);
		REQUIRE(!dwarf1.m_canPickup.exists());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "haul");
	}
	SUBCASE("hand cart haul strategy")
	{
		Block& destination = area.getBlock(5, 5, 2);
		Block& chunkLocation = area.getBlock(1, 5, 2);
		Item& chunk1 = simulation.createItemGeneric(chunk, lead, 1u);
		chunk1.setLocation(chunkLocation);
		REQUIRE(!dwarf1.m_canPickup.canPickupAny(chunk1));
		Item& cart = simulation.createItemNongeneric(ItemType::byName("cart"), MaterialType::byName("poplar wood"), 50u, 0);
		Block& cartLocation = area.getBlock(7, 7, 2);
		cart.setLocation(cartLocation);
		area.m_hasHaulTools.registerHaulTool(cart);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(std::vector<Actor*>({&dwarf1}), chunk1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		REQUIRE(project.getWorkers().contains(&dwarf1));
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(dwarf1, cart, chunk1, project.getMinimumHaulSpeed()) > 0);
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Cart);
		// Another step to find the path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, cartLocation);
		REQUIRE(dwarf1.m_canLead.isLeading(cart));
		// Another step to path from cart to chunk.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, chunkLocation);
		REQUIRE(cart.m_hasCargo.getContents().size() == 1);
		REQUIRE(**(cart.m_hasCargo.getItems().begin()) == chunk1);
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(destination.m_hasItems.getCount(chunk, lead) == 1);
		REQUIRE(!dwarf1.m_canPickup.exists());
		REQUIRE(cart.m_hasCargo.empty());
		REQUIRE(!cart.m_reservable.isFullyReserved(&faction));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "haul");
	}
	SUBCASE("team haul strategy")
	{
		Block& destination = area.getBlock(8, 8, 2);
		Block& chunkLocation = area.getBlock(1, 5, 2);
		Item& chunk1 = simulation.createItemGeneric(chunk, lead, 1u);
		chunk1.setLocation(chunkLocation);
		REQUIRE(!dwarf1.m_canPickup.canPickupAny(chunk1));
		Actor& dwarf2 = simulation.createActor(dwarf, area.getBlock(1, 2, 2));
		dwarf2.setFaction(&faction);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(std::vector<Actor*>({&dwarf1, &dwarf2}), chunk1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		// Another step to find the paths.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Team);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "haul");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "haul");
		// Get into starting positions.
		simulation.fastForwardUntillActorHasNoDestination(dwarf2);
		if(!dwarf1.isAdjacentTo(chunk1))
			simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, chunk1);
		if(dwarf1.m_canMove.getDestination() != nullptr)
			simulation.fastForwardUntillActorHasNoDestination(dwarf1);
		REQUIRE(dwarf2.m_canFollow.isFollowing());
		REQUIRE(dwarf1.m_canLead.isLeading());
		// Another step to path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(destination.m_hasItems.getCount(chunk, lead) == 1);
		REQUIRE(!dwarf2.m_canFollow.isFollowing());
		REQUIRE(!dwarf1.m_canLead.isLeading());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "haul");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "haul");
	}
	SUBCASE("panniers haul strategy")
	{
		Block& destination = area.getBlock(8, 8, 2);
		Block& chunkLocation = area.getBlock(1, 5, 2);
		Item& chunk1 = simulation.createItemGeneric(chunk, gold, 1u);
		chunk1.setLocation(chunkLocation);
		REQUIRE(!dwarf1.m_canPickup.canPickupAny(chunk1));
		Block& donkeyLocation = area.getBlock(1, 2, 2);
		Actor& donkey1 = simulation.createActor(donkey, donkeyLocation);
		area.m_hasHaulTools.registerYokeableActor(donkey1);
		Block& panniersLocation = area.getBlock(5, 1, 2);
		Item& panniers1 = simulation.createItemNongeneric(panniers, poplarWood, 3u, 0);
		panniers1.setLocation(panniersLocation);
		area.m_hasHaulTools.registerHaulTool(panniers1);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(std::vector<Actor*>({&dwarf1}), chunk1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Panniers);
		// Another step to find the paths.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getPath().size() != 0);
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, panniersLocation);
		simulation.doStep();
		REQUIRE(dwarf1.m_canPickup.isCarrying(panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, donkeyLocation);
		// Find path.
		simulation.doStep();
		REQUIRE(!dwarf1.m_canPickup.exists());
		REQUIRE(donkey1.m_equipmentSet.contains(panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, chunkLocation);
		// Find path.
		simulation.doStep();
		REQUIRE(panniers1.m_hasCargo.contains(chunk1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(destination.m_hasItems.getCount(chunk, gold) == 1);
		REQUIRE(!donkey1.m_reservable.isFullyReserved(&faction));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "haul");
	}
	SUBCASE("animal cart haul strategy")
	{
		Block& destination = area.getBlock(5, 5, 2);
		Block& boulderLocation = area.getBlock(1, 5, 2);
		Item& boulder1 = simulation.createItemGeneric(boulder, peridotite, 1u);
		boulder1.setLocation(boulderLocation);
		Block& donkeyLocation = area.getBlock(4, 3, 2);
		Actor& donkey1 = simulation.createActor(donkey, donkeyLocation);
		area.m_hasHaulTools.registerYokeableActor(donkey1);
		Block& cartLocation = area.getBlock(5, 1, 2);
		Item& cart1 = simulation.createItemNongeneric(cart, poplarWood, 3u, 0);
		cart1.setLocation(cartLocation);
		area.m_hasHaulTools.registerHaulTool(cart1);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(std::vector<Actor*>({&dwarf1}), boulder1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::AnimalCart);
		// Another step to find the paths.
		simulation.doStep();
		REQUIRE(donkey1.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, donkey1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canLead.isLeading(donkey1));
		REQUIRE(dwarf1.m_canMove.getPath().size() != 0);
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, cartLocation);
		REQUIRE(donkey1.m_canLead.isLeading(cart1));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, boulderLocation);
		REQUIRE(cart1.m_hasCargo.contains(boulder1));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(destination.m_hasItems.getCount(boulder, peridotite) == 1);
		REQUIRE(!donkey1.m_reservable.isFullyReserved(&faction));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "haul");
	}
	SUBCASE("team hand cart haul strategy")
	{
		Block& destination = area.getBlock(5, 5, 2);
		Block& cargoLocation = area.getBlock(1, 5, 2);
		Item& cargo1 = simulation.createItemGeneric(boulder, peridotite, 1u);
		cargo1.setLocation(cargoLocation);
		Block& origin2 = area.getBlock(4, 3, 2);
		Actor& dwarf2 = simulation.createActor(dwarf, origin2);
		dwarf2.setFaction(&faction);
		Block& cartLocation = area.getBlock(5, 1, 2);
		Item& cart1 = simulation.createItemNongeneric(cart, poplarWood, 3u, 0);
		cart1.setLocation(cartLocation);
		area.m_hasHaulTools.registerHaulTool(cart1);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(std::vector<Actor*>({&dwarf1, &dwarf2}), cargo1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::TeamCart);
		ProjectWorker& projectWorker2 = project.getProjectWorkerFor(dwarf2);
		REQUIRE(projectWorker2.haulSubproject != nullptr);
		REQUIRE(projectWorker2.haulSubproject->getHaulStrategy() == HaulStrategy::TeamCart);
		// Another step to find the paths.
		simulation.doStep();
		// Get into starting positions.
		if(dwarf1.m_canMove.getPath().empty() || dwarf2.m_canMove.getPath().empty())
			// The two dwarves pathed to the same place, one needs to repath.
			simulation.doStep();
		Block& destination1 = *dwarf1.m_canMove.getDestination();
		Block& destination2 = *dwarf2.m_canMove.getDestination();
		REQUIRE(cart1.isAdjacentTo(destination1));
		REQUIRE(cart1.isAdjacentTo(destination2));
		REQUIRE(destination1 != destination2);
		simulation.fastForwardUntillActorIsAtDestination(dwarf1, destination1);
		if(dwarf2.m_location != &destination2)
			simulation.fastForwardUntillActorIsAtDestination(dwarf2, destination2);
		REQUIRE(dwarf1.m_canLead.isLeading(cart1));
		REQUIRE(cart1.m_canLead.isLeading(dwarf2));
		REQUIRE(dwarf2.m_canFollow.isFollowing());
		// Find path
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, cargoLocation);
		REQUIRE(cart1.m_hasCargo.contains(cargo1));
		// Path a detour around dwarf2 again.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(destination.m_hasItems.getCount(boulder, peridotite) == 1);
		REQUIRE(!cart1.m_reservable.isFullyReserved(&faction));
		REQUIRE(!dwarf1.m_canLead.isLeading());
		REQUIRE(!dwarf2.m_canFollow.isFollowing());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "haul");
	}
}
