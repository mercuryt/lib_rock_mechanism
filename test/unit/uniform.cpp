#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/itemType.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/types.h"
#include "../../engine/objectives/uniform.h"
#include "../../engine/items/items.h"
TEST_CASE("uniform")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
	UniformElement pantsElement(ItemType::byName("pants"));
	UniformElement shirtElement(ItemType::byName("shirt"));
	UniformElement twoBeltsElement(ItemType::byName("belt"), Quantity::create(2));
	Uniform basic = Uniform(L"basic", {pantsElement, shirtElement, twoBeltsElement});
	ActorIndex actor = actors.create({
		.species=AnimalSpecies::byName("dwarf"),
		.location=blocks.getIndex_i(5,5,1),
	});
	actors.uniform_set(actor, basic);
	REQUIRE(actors.objective_getCurrent<UniformObjective>(actor).getTypeId() == ObjectiveType::getIdByName("uniform"));
	UniformObjective& objective = static_cast<UniformObjective&>(actors.objective_getCurrent<UniformObjective>(actor));
	ItemIndex pants = items.create({.itemType=ItemType::byName("pants"), .materialType=MaterialType::byName("cotton"), .location=blocks.getIndex_i(8,1,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	ItemIndex shirt = items.create({.itemType=ItemType::byName("shirt"), .materialType=MaterialType::byName("cotton"), .location=blocks.getIndex_i(1,6,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	ItemIndex belt = items.create({.itemType=ItemType::byName("belt"), .materialType=MaterialType::byName("leather"), .location=blocks.getIndex_i(9,2,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	ItemIndex belt2 = items.create({.itemType=ItemType::byName("belt"), .materialType=MaterialType::byName("leather"), .location=blocks.getIndex_i(9,3,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	simulation.doStep();
	BlockIndex destination = actors.move_getDestination(actor);
	REQUIRE(items.isAdjacentToLocation(shirt, destination));
	REQUIRE(objective.getItem() == shirt);
	simulation.fastForwardUntillActorHasEquipment(area, actor, shirt);
	simulation.doStep();
	REQUIRE(actors.equipment_containsItem(actor, shirt));
	REQUIRE(items.getLocation(shirt).empty());
	REQUIRE(items.isAdjacentToLocation(shirt, actors.move_getDestination(actor)));
	simulation.fastForwardUntillActorHasEquipment(area, actor, pants);
	simulation.doStep();
	REQUIRE(actors.equipment_containsItem(actor, pants));
	simulation.fastForwardUntillActorHasEquipment(area, actor, belt);
	simulation.doStep();
	REQUIRE(actors.equipment_containsItem(actor, belt));
	if(!actors.equipment_containsItem(actor, belt2))
		simulation.fastForwardUntillActorHasEquipment(area, actor, belt2);
	REQUIRE(actors.objective_getCurrent<UniformObjective>(actor).getTypeId() != ObjectiveType::getIdByName("uniform"));
}
