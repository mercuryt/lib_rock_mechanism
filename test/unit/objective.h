#include "../../lib/doctest.h"
#include "../../src/objective.h"
#include "../../src/animalSpecies.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/simulation.h"

#include <memory>

struct TestObjective final : public Objective
{
	bool& x;
	TestObjective(uint32_t priority, bool& ax) : Objective(priority), x(ax) { }
	void execute() { x = !x; }
	void cancel() { }
};

TEST_CASE("objective")
{
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	simulation::init();
	Area area(10,10,10,280);
	Actor& actor = Actor::create(dwarf, area.m_blocks[5][5][5]);
	bool x = false;
	std::unique_ptr<Objective> objective = std::make_unique<TestObjective>(1, x);
	Objective* ptr = objective.get();
	actor.m_hasObjectives.addTaskToEnd(std::move(objective));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr);
	CHECK(x);
	std::unique_ptr<Objective> objective2 = std::make_unique<TestObjective>(2, x);
	Objective* ptr2 = objective2.get();
	actor.m_hasObjectives.addNeed(std::move(objective2));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr2);
	CHECK(!x);
	actor.m_hasObjectives.objectiveComplete(*ptr2);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr);
	CHECK(x);
	std::unique_ptr<Objective> objective3 = std::make_unique<TestObjective>(2, x);
	Objective* ptr3 = objective3.get();
	actor.m_hasObjectives.addTaskToEnd(std::move(objective3));
	actor.m_hasObjectives.objectiveComplete(*ptr);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr3);
	CHECK(!x);
	std::unique_ptr<Objective> objective4 = std::make_unique<TestObjective>(1, x);
	Objective* ptr4 = objective4.get();
	actor.m_hasObjectives.addNeed(std::move(objective4));
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr3);
	CHECK(!x);
	actor.m_hasObjectives.objectiveComplete(*ptr3);
	CHECK(&actor.m_hasObjectives.getCurrent() == ptr4);
	CHECK(x);
}
