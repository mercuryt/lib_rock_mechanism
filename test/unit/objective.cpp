#include "../../lib/doctest.h"
#include "../../engine/objective.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"

#include <memory>

struct TestTaskObjective final : public Objective
{
	bool& x;
	TestTaskObjective(Actor& a, uint32_t priority, bool& ax) : Objective(a, priority), x(ax) { }
	void execute() { x = !x; }
	void cancel() { }
	void delay() { }
	void reset() { }
	void detour() { }
	std::string name() const { return "test task"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Construct; }
};

struct TestNeedObjective final : public Objective
{
	bool& x;
	TestNeedObjective(Actor& a, uint32_t priority, bool& ax) : Objective(a, priority), x(ax) { }
	void execute() { x = !x; }
	void cancel() { }
	void delay() { }
	void reset() { }
	void detour() { }
	std::string name() const { return "test need"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
};

TEST_CASE("objective")
{
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actor& actor = simulation.m_hasActors->createActor(dwarf, area.getBlocks().getIndex({5, 5, 1}));
	bool x = false;
	std::unique_ptr<Objective> objective = std::make_unique<TestTaskObjective>(actor, 1, x);
	Objective* ptr = objective.get();
	actor.m_hasObjectives.addTaskToEnd(std::move(objective));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr);
	CHECK(x);
	std::unique_ptr<Objective> objective2 = std::make_unique<TestNeedObjective>(actor, 2, x);
	Objective* ptr2 = objective2.get();
	actor.m_hasObjectives.addNeed(std::move(objective2));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr2);
	CHECK(!x);
	actor.m_hasObjectives.objectiveComplete(*ptr2);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr);
	CHECK(x);
	std::unique_ptr<Objective> objective3 = std::make_unique<TestTaskObjective>(actor, 2, x);
	Objective* ptr3 = objective3.get();
	actor.m_hasObjectives.addTaskToEnd(std::move(objective3));
	actor.m_hasObjectives.objectiveComplete(*ptr);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr3);
	CHECK(!x);
	std::unique_ptr<Objective> objective4 = std::make_unique<TestNeedObjective>(actor, 1, x);
	Objective* ptr4 = objective4.get();
	actor.m_hasObjectives.addNeed(std::move(objective4));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr3);
	CHECK(!x);
	actor.m_hasObjectives.objectiveComplete(*ptr3);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr4);
	CHECK(x);
}
