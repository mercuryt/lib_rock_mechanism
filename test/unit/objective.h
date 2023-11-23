#include "../../lib/doctest.h"
#include "../../src/objective.h"
#include "../../src/animalSpecies.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"

#include <memory>

struct TestTaskObjective final : public Objective
{
	bool& x;
	TestTaskObjective(uint32_t priority, bool& ax) : Objective(priority), x(ax) { }
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
	TestNeedObjective(uint32_t priority, bool& ax) : Objective(priority), x(ax) { }
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
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actor& actor = simulation.createActor(dwarf, area.m_blocks[5][5][1]);
	bool x = false;
	std::unique_ptr<Objective> objective = std::make_unique<TestTaskObjective>(1, x);
	Objective* ptr = objective.get();
	actor.m_hasObjectives.addTaskToEnd(std::move(objective));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr);
	CHECK(x);
	std::unique_ptr<Objective> objective2 = std::make_unique<TestNeedObjective>(2, x);
	Objective* ptr2 = objective2.get();
	actor.m_hasObjectives.addNeed(std::move(objective2));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr2);
	CHECK(!x);
	actor.m_hasObjectives.objectiveComplete(*ptr2);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr);
	CHECK(x);
	std::unique_ptr<Objective> objective3 = std::make_unique<TestTaskObjective>(2, x);
	Objective* ptr3 = objective3.get();
	actor.m_hasObjectives.addTaskToEnd(std::move(objective3));
	actor.m_hasObjectives.objectiveComplete(*ptr);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr3);
	CHECK(!x);
	std::unique_ptr<Objective> objective4 = std::make_unique<TestNeedObjective>(1, x);
	Objective* ptr4 = objective4.get();
	actor.m_hasObjectives.addNeed(std::move(objective4));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr3);
	CHECK(!x);
	actor.m_hasObjectives.objectiveComplete(*ptr3);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr4);
	CHECK(x);
}
