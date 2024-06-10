#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/areaBuilderUtil.h"
#include "types.h"
TEST_CASE("uniform")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
	UniformElement pantsElement(ItemType::byName("pants"));
	UniformElement shirtElement(ItemType::byName("shirt"));
	UniformElement twoBeltsElement(ItemType::byName("belt"), 2u);
	Uniform basic = Uniform(L"basic", {pantsElement, shirtElement, twoBeltsElement});
	Actor& actor = simulation.m_hasActors->createActor({
		.species=AnimalSpecies::byName("dwarf"),
		.location=blocks.getIndex({5,5,1}),
		.area=&area,
	});
	actor.m_hasUniform.set(basic);
	REQUIRE(actor.m_hasObjectives.hasCurrent());
	REQUIRE(actor.m_hasObjectives.getCurrent().getObjectiveTypeId() == ObjectiveTypeId::Uniform);
	UniformObjective& objective = static_cast<UniformObjective&>(actor.m_hasObjectives.getCurrent());
	Item& pants = simulation.m_hasItems->createItemNongeneric(ItemType::byName("pants"), MaterialType::byName("cotton"), 10, 10);
	pants.setLocation(blocks.getIndex({8,1,1}), &area);
	Item& shirt = simulation.m_hasItems->createItemNongeneric(ItemType::byName("shirt"), MaterialType::byName("cotton"), 10, 10);
	shirt.setLocation(blocks.getIndex({1,6,1}), &area);
	Item& belt = simulation.m_hasItems->createItemNongeneric(ItemType::byName("belt"), MaterialType::byName("leather"), 10, 10);
	belt.setLocation(blocks.getIndex({9,2,1}), &area);
	Item& belt2 = simulation.m_hasItems->createItemNongeneric(ItemType::byName("belt"), MaterialType::byName("leather"), 10, 10);
	belt2.setLocation(blocks.getIndex({9,3,1}), &area);
	simulation.doStep();
	BlockIndex destination = actor.m_canMove.getDestination();
	REQUIRE(shirt.isAdjacentTo(destination));
	REQUIRE(objective.getItem() == &shirt);
	simulation.fastForwardUntillActorHasEquipment(actor, shirt);
	simulation.doStep();
	REQUIRE(actor.m_equipmentSet.contains(shirt));
	REQUIRE(shirt.m_location == BLOCK_INDEX_MAX);
	REQUIRE(pants.isAdjacentTo(actor.m_canMove.getDestination()));
	simulation.fastForwardUntillActorHasEquipment(actor, pants);
	simulation.doStep();
	REQUIRE(actor.m_equipmentSet.contains(pants));
	simulation.fastForwardUntillActorHasEquipment(actor, belt);
	simulation.doStep();
	REQUIRE(actor.m_equipmentSet.contains(belt));
	if(!actor.m_equipmentSet.contains(belt2))
		simulation.fastForwardUntillActorHasEquipment(actor, belt2);
	REQUIRE(actor.m_hasObjectives.getCurrent().getObjectiveTypeId() != ObjectiveTypeId::Uniform);
}
