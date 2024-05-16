#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/materialType.h"
#include "../../engine/project.h"
#include "../../engine/config.h"
#include "../../engine/haul.h"
#include "../../engine/objectives/equipItem.h"
#include "../../engine/objectives/unequipItem.h"
#include "../../engine/objectives/giveItem.h"
#include <functional>
#include <memory>
TEST_CASE("equip and unequip")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Block& destination = area.getBlock(8,2,1);
	Actor& dwarf1 = simulation.createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.location=&area.getBlock(1, 1, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	Item& longsword = simulation.m_hasItems->createItemNongeneric(ItemType::byName("long sword"), MaterialType::byName("bronze"), 20, 10);
	Block& swordLocation = area.getBlock(8,8,1);
	longsword.setLocation(swordLocation);
	std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(dwarf1, longsword);
	dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
	REQUIRE(dwarf1.m_canMove.hasThreadedTask());
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, swordLocation);
	REQUIRE(swordLocation.m_hasItems.empty());
	REQUIRE(dwarf1.m_equipmentSet.contains(longsword));
	REQUIRE(dwarf1.getActionDescription() != L"equip");
	std::unique_ptr<Objective> objective2 = std::make_unique<UnequipItemObjective>(dwarf1, longsword, destination);
	dwarf1.m_hasObjectives.addTaskToStart(std::move(objective2));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, destination);
	REQUIRE(dwarf1.m_equipmentSet.empty());
	REQUIRE(longsword.m_location == &destination);
	REQUIRE(dwarf1.getActionDescription() != L"equip");
}
TEST_CASE("give item")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actor& dwarf1 = simulation.createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.location=&area.getBlock(1, 1, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	Actor& dwarf2 = simulation.createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.location=&area.getBlock(6, 6, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	Item& longsword = simulation.m_hasItems->createItemNongeneric(ItemType::byName("long sword"), MaterialType::byName("bronze"), 20, 10);
	dwarf1.m_equipmentSet.addEquipment(longsword);
	std::unique_ptr<Objective> objective = std::make_unique<GiveItemObjective>(dwarf1, longsword, dwarf2);
	dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
	REQUIRE(dwarf1.m_canMove.hasThreadedTask());
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToHasShape(dwarf1, dwarf2);
	REQUIRE(!dwarf1.m_equipmentSet.contains(longsword));
	REQUIRE(dwarf2.m_equipmentSet.contains(longsword));
	REQUIRE(dwarf1.getActionDescription() != L"give item");
}
