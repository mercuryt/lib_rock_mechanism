#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/areaBuilderUtil.h"
TEST_CASE("uniform")
{
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
	UniformElement pantsElement(ItemType::byName("pants"));
	UniformElement shirtElement(ItemType::byName("shirt"));
	UniformElement twoBeltsElement(ItemType::byName("belt"), 2u);
	Uniform basic = Uniform(L"basic", {pantsElement, shirtElement, twoBeltsElement});
	Actor& actor = simulation.createActor(AnimalSpecies::byName("dwarf"), area.getBlock(5,5,1));
	actor.m_hasUniform.set(basic);
	REQUIRE(actor.m_hasObjectives.hasCurrent());
	REQUIRE(actor.m_hasObjectives.getCurrent().getObjectiveTypeId() == ObjectiveTypeId::Uniform);
	UniformObjective& objective = static_cast<UniformObjective&>(actor.m_hasObjectives.getCurrent());
	Item& pants = simulation.m_hasItems->createItemNongeneric(ItemType::byName("pants"), MaterialType::byName("cotton"), 10, 10);
	pants.setLocation(area.getBlock(8,1,1));
	Item& shirt = simulation.m_hasItems->createItemNongeneric(ItemType::byName("shirt"), MaterialType::byName("cotton"), 10, 10);
	shirt.setLocation(area.getBlock(1,6,1));
	Item& belt = simulation.m_hasItems->createItemNongeneric(ItemType::byName("belt"), MaterialType::byName("leather"), 10, 10);
	belt.setLocation(area.getBlock(9,2,1));
	Item& belt2 = simulation.m_hasItems->createItemNongeneric(ItemType::byName("belt"), MaterialType::byName("leather"), 10, 10);
	belt2.setLocation(area.getBlock(9,3,1));
	simulation.doStep();
	Block* destination = actor.m_canMove.getDestination();
	REQUIRE(destination->isAdjacentTo(shirt));
	REQUIRE(objective.getItem() == &shirt);
	simulation.fastForwardUntillActorHasEquipment(actor, shirt);
	simulation.doStep();
	REQUIRE(actor.m_equipmentSet.contains(shirt));
	REQUIRE(!shirt.m_location);
	REQUIRE(actor.m_canMove.getDestination()->isAdjacentTo(pants));
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
