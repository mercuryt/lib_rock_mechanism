#include "../../lib/doctest.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
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
	MaterialTypeId marble = MaterialType::byName(L"marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	BlockIndex destination = blocks.getIndex_i(8,2,1);
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName(L"dwarf"),
		.location=blocks.getIndex_i(1, 1, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	BlockIndex swordLocation = blocks.getIndex_i(8,8,1);
	ItemIndex longsword = items.create({.itemType=ItemType::byName(L"long sword"), .materialType=MaterialType::byName(L"bronze"), .location=swordLocation, .quality=Quality::create(20), .percentWear=Percent::create(10)});
	ItemReference longswordRef = items.m_referenceData.getReference(longsword);
	CHECK(items.getBlocks(longsword).size() == 1);
	CHECK(items.getBlocks(longsword).front() == swordLocation);
	std::unique_ptr<Objective> objective = std::make_unique<EquipItemObjective>(longswordRef);
	actors.objective_addTaskToStart(dwarf1, std::move(objective));
	CHECK(actors.move_hasPathRequest(dwarf1));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, swordLocation);
	CHECK(blocks.item_empty(swordLocation));
	CHECK(actors.equipment_containsItem(dwarf1, longsword));
	CHECK(actors.getActionDescription(dwarf1) != L"equip");
	std::unique_ptr<Objective> objective2 = std::make_unique<UnequipItemObjective>(longswordRef, destination);
	actors.objective_addTaskToStart(dwarf1, std::move(objective2));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, destination);
	CHECK(actors.equipment_getAll(dwarf1).empty());
	CHECK(items.getLocation(longsword) == destination);
	CHECK(actors.getActionDescription(dwarf1) != L"equip");
}
TEST_CASE("give item")
{
	MaterialTypeId marble = MaterialType::byName(L"marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName(L"dwarf"),
		.location=blocks.getIndex_i(1, 1, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	ActorIndex dwarf2 = actors.create({
		.species=AnimalSpecies::byName(L"dwarf"),
		.location=blocks.getIndex_i(6, 6, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	ItemIndex longsword = items.create({.itemType=ItemType::byName(L"long sword"), .materialType=MaterialType::byName(L"bronze"), .quality=Quality::create(20), .percentWear=Percent::create(10)});
	actors.equipment_add(dwarf1, longsword);
	std::unique_ptr<Objective> objective = std::make_unique<GiveItemObjective>(area, longsword, dwarf2);
	actors.objective_addTaskToStart(dwarf1, std::move(objective));
	CHECK(actors.move_hasPathRequest(dwarf1));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToActor(area, dwarf1, dwarf2);
	CHECK(!actors.equipment_containsItem(dwarf1, longsword));
	CHECK(actors.equipment_containsItem(dwarf2, longsword));
	CHECK(actors.getActionDescription(dwarf1) != L"give item");
}
