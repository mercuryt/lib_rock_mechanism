#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
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
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	BlockIndex destination = blocks.getIndex({8,2,1});
	Actor& dwarf1 = simulation.m_hasActors->createActor({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({1, 1, 1}),
		.area=&area,
		.hasCloths=false,
		.hasSidearm=false
	});
	Item& longsword = simulation.m_hasItems->createItemNongeneric(ItemType::byName("long sword"), MaterialType::byName("bronze"), 20, 10);
	BlockIndex swordLocation = blocks.getIndex({8,8,1});
	longsword.setLocation(swordLocation, &area);
	std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(dwarf1, longsword);
	dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
	REQUIRE(dwarf1.m_canMove.hasThreadedTask());
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, swordLocation);
	REQUIRE(blocks.item_empty(swordLocation));
	REQUIRE(dwarf1.m_equipmentSet.contains(longsword));
	REQUIRE(dwarf1.getActionDescription() != L"equip");
	std::unique_ptr<Objective> objective2 = std::make_unique<UnequipItemObjective>(dwarf1, longsword, destination);
	dwarf1.m_hasObjectives.addTaskToStart(std::move(objective2));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, destination);
	REQUIRE(dwarf1.m_equipmentSet.empty());
	REQUIRE(longsword.m_location == destination);
	REQUIRE(dwarf1.getActionDescription() != L"equip");
}
TEST_CASE("give item")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actor& dwarf1 = simulation.m_hasActors->createActor({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({1, 1, 1}),
		.area=&area,
		.hasCloths=false,
		.hasSidearm=false
	});
	Actor& dwarf2 = simulation.m_hasActors->createActor({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({6, 6, 1}),
		.area=&area,
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
