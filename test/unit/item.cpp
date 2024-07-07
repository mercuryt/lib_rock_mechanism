#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/materialType.h"
#include "../../engine/project.h"
#include "../../engine/config.h"
#include "../../engine/haul.h"
#include "../../engine/itemType.h"
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
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({1, 1, 1}),
		.hasCloths=false,
		.hasSidearm=false
	});
	BlockIndex swordLocation = blocks.getIndex({8,8,1});
	ItemIndex longsword = items.create({.itemType=ItemType::byName("long sword"), .materialType=MaterialType::byName("bronze"), .location=swordLocation, .quality=20, .percentWear=10});
	std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(dwarf1, longsword);
	actors.objective_addTaskToStart(dwarf1, std::move(objective));
	REQUIRE(actors.move_hasPathRequest(dwarf1));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, swordLocation);
	REQUIRE(blocks.item_empty(swordLocation));
	REQUIRE(actors.equipment_containsItem(dwarf1, longsword));
	REQUIRE(actors.getActionDescription(dwarf1) != L"equip");
	std::unique_ptr<Objective> objective2 = std::make_unique<UnequipItemObjective>(dwarf1, longsword, destination);
	actors.objective_addTaskToStart(dwarf1, std::move(objective2));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, destination);
	REQUIRE(actors.equipment_getAll(dwarf1).empty());
	REQUIRE(items.getLocation(longsword) == destination);
	REQUIRE(actors.getActionDescription(dwarf1) != L"equip");
}
TEST_CASE("give item")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({1, 1, 1}),
		.hasCloths=false,
		.hasSidearm=false
	});
	ActorIndex dwarf2 = actors.create({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({6, 6, 1}),
		.hasCloths=false,
		.hasSidearm=false
	});
	ItemIndex longsword = items.create({.itemType=ItemType::byName("long sword"), .materialType=MaterialType::byName("bronze"), .quality=20, .percentWear=10});
	actors.equipment_add(dwarf1, longsword);
	std::unique_ptr<Objective> objective = std::make_unique<GiveItemObjective>(area, dwarf1, longsword, dwarf2);
	actors.objective_addTaskToStart(dwarf1, std::move(objective));
	REQUIRE(actors.move_hasPathRequest(dwarf1));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToActor(area, dwarf1, dwarf2);
	REQUIRE(!actors.equipment_containsItem(dwarf1, longsword));
	REQUIRE(actors.equipment_containsItem(dwarf2, longsword));
	REQUIRE(actors.getActionDescription(dwarf1) != L"give item");
}
