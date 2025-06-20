#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/numericTypes/types.h"
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
	UniformElement pantsElement = UniformElement::create(ItemType::byName("pants"));
	UniformElement shirtElement = UniformElement::create(ItemType::byName("shirt"));
	UniformElement twoBeltsElement = UniformElement::create(ItemType::byName("belt"), Quantity::create(2));
	Uniform basic = Uniform("basic", {pantsElement, shirtElement, twoBeltsElement});
	ActorIndex actor = actors.create({
		.species=AnimalSpecies::byName("dwarf"),
		.location=blocks.getIndex_i(5,5,1),
	});
	actors.uniform_set(actor, basic);
	CHECK(actors.objective_getCurrentName(actor) == "uniform");
	UniformObjective& objective = actors.objective_getCurrent<UniformObjective>(actor);
	ItemIndex pants = items.create({.itemType=ItemType::byName("pants"), .materialType=MaterialType::byName("cotton"), .location=blocks.getIndex_i(8,1,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	ItemIndex shirt = items.create({.itemType=ItemType::byName("shirt"), .materialType=MaterialType::byName("cotton"), .location=blocks.getIndex_i(1,6,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	ItemIndex belt = items.create({.itemType=ItemType::byName("belt"), .materialType=MaterialType::byName("leather"), .location=blocks.getIndex_i(9,2,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	ItemIndex belt2 = items.create({.itemType=ItemType::byName("belt"), .materialType=MaterialType::byName("leather"), .location=blocks.getIndex_i(9,3,1), .quality=Quality::create(10), .percentWear=Percent::create(10)});
	simulation.doStep();
	BlockIndex destination = actors.move_getDestination(actor);
	CHECK(items.isAdjacentToLocation(shirt, destination));
	CHECK(objective.getItem().getIndex(items.m_referenceData) == shirt);
	simulation.fastForwardUntillActorHasEquipment(area, actor, shirt);
	simulation.doStep();
	CHECK(actors.equipment_containsItem(actor, shirt));
	CHECK(items.getLocation(shirt).empty());
	CHECK(items.isAdjacentToLocation(pants, actors.move_getDestination(actor)));
	simulation.fastForwardUntillActorHasEquipment(area, actor, pants);
	simulation.doStep();
	CHECK(actors.equipment_containsItem(actor, pants));
	simulation.fastForwardUntillActorHasEquipment(area, actor, belt);
	simulation.doStep();
	CHECK(actors.equipment_containsItem(actor, belt));
	if(!actors.equipment_containsItem(actor, belt2))
		simulation.fastForwardUntillActorHasEquipment(area, actor, belt2);
	CHECK(actors.objective_getCurrentName(actor) != "uniform");
}
