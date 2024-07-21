#include "../../lib/doctest.h"
#include "../../engine/objective.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"

#include <memory>

struct TestTaskObjective final : public Objective
{
	bool& x;
	TestTaskObjective(ActorIndex a, uint32_t priority, bool& ax) : Objective(a, priority), x(ax) { }
	void execute(Area&) { x = !x; }
	void cancel(Area&) { }
	void delay(Area&) { }
	void reset(Area&) { }
	void detour(Area&) { }
	std::string name() const { return "test task"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Construct; }
};

struct TestNeedObjective final : public Objective
{
	bool& x;
	TestNeedObjective(ActorIndex a, uint32_t priority, bool& ax) : Objective(a, priority), x(ax) { }
	void execute(Area&) { x = !x; }
	void cancel(Area&) { }
	void delay(Area&) { }
	void reset(Area&) { }
	void detour(Area&) { }
	std::string name() const { return "test need"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Dig; }
};

TEST_CASE("objective")
{
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=area.getBlocks().getIndex({5, 5, 1}),
	});
	bool x = false;
	// Add objective to end of empty task list and it becomes current objective.
	std::unique_ptr<Objective> objective = std::make_unique<TestTaskObjective>(actor, 1, x);
	Objective* ptr = objective.get();
	actors.objective_addTaskToEnd(actor, std::move(objective));
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr);
	CHECK(x);
	// Add a need with higher prioirity then the task and it usurps it as current objective.
	std::unique_ptr<Objective> objective2 = std::make_unique<TestNeedObjective>(actor, 2, x);
	Objective* ptr2 = objective2.get();
	actors.objective_addNeed(actor, std::move(objective2));
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr2);
	CHECK(!x);
	// Fulfull the need and the task becomes current objective again.
	actors.objective_complete(actor, *ptr2);
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr);
	CHECK(x);
	// Add task to end and then complete the current task, new task becomes current.
	std::unique_ptr<Objective> objective3 = std::make_unique<TestTaskObjective>(actor, 2, x);
	Objective* ptr3 = objective3.get();
	actors.objective_addTaskToEnd(actor, std::move(objective3));
	actors.objective_complete(actor, *ptr);
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr3);
	CHECK(!x);
	// Add need with lower priority then the current task and it does not usurp.
	std::unique_ptr<Objective> objective4 = std::make_unique<TestNeedObjective>(actor, 1, x);
	Objective* ptr4 = objective4.get();
	actors.objective_addNeed(actor, std::move(objective4));
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr3);
	CHECK(!x);
	// Complete task and lower priority need becomes current.
	actors.objective_complete(actor, *ptr3);
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr4);
	CHECK(x);
}
