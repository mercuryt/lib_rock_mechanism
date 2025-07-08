#include "../../lib/doctest.h"
#include "../../engine/objective.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/portables.hpp"
#include "numericTypes/types.h"

#include <memory>
#include "objective.h"

struct TestTaskObjective final : public Objective
{
	bool& x;
	TestTaskObjective(Priority priority, bool& ax) : Objective(priority), x(ax) { }
	void execute(Area&, const ActorIndex&) { x = !x; }
	void cancel(Area&, const ActorIndex&) { }
	void delay(Area&, const ActorIndex&) { }
	void reset(Area&, const ActorIndex&) { }
	void detour(Area&, const ActorIndex&) { }
	std::string name() const { return "test task"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveType::getIdByName("construct"); }
};

struct TestNeedObjective final : public Objective
{
	bool& x;
	TestNeedObjective(Priority priority, bool& ax) : Objective(priority), x(ax) { }
	void execute(Area&, const ActorIndex&) { x = !x; }
	void cancel(Area&, const ActorIndex&) { }
	void delay(Area&, const ActorIndex&) { }
	void reset(Area&, const ActorIndex&) { }
	void detour(Area&, const ActorIndex&) { }
	std::string name() const { return "test need"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveType::getIdByName("dig"); }
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::test; }
};

TEST_CASE("objective")
{
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static MaterialTypeId marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=area.getSpace().getIndex_i(5, 5, 1),
	});
	bool x = false;
	// Add objective to end of empty task list and it becomes current objective.
	std::unique_ptr<Objective> objective = std::make_unique<TestTaskObjective>(Priority::create(1), x);
	Objective* ptr = objective.get();
	actors.objective_addTaskToEnd(actor, std::move(objective));
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr);
	CHECK(x);
	// Add a need with higher prioirity then the task and it usurps it as current objective.
	std::unique_ptr<Objective> objective2 = std::make_unique<TestNeedObjective>(Priority::create(2), x);
	Objective* ptr2 = objective2.get();
	actors.objective_addNeed(actor, std::move(objective2));
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr2);
	CHECK(!x);
	// Fulfull the need and the task becomes current objective again.
	actors.objective_complete(actor, *ptr2);
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr);
	CHECK(x);
	// Add task to end and then complete the current task, new task becomes current.
	std::unique_ptr<Objective> objective3 = std::make_unique<TestTaskObjective>(Priority::create(2), x);
	Objective* ptr3 = objective3.get();
	actors.objective_addTaskToEnd(actor, std::move(objective3));
	actors.objective_complete(actor, *ptr);
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr3);
	CHECK(!x);
	// Add need with lower priority then the current task and it does not usurp.
	std::unique_ptr<Objective> objective4 = std::make_unique<TestNeedObjective>(Priority::create(1), x);
	Objective* ptr4 = objective4.get();
	actors.objective_addNeed(actor, std::move(objective4));
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr3);
	CHECK(!x);
	// Complete task and lower priority need becomes current.
	actors.objective_complete(actor, *ptr3);
	CHECK(&actors.objective_getCurrent<Objective>(actor) == ptr4);
	CHECK(x);
}
