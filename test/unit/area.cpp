#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/actors/actors.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/space/space.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/config/config.h"
#include "numericTypes/types.h"

TEST_CASE("Area")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10, 10, 10);
	area.m_hasRain.disable();
	// Prevent water freezing.
	area.m_hasTemperature.setAmbient(area, FluidType::getFreezingPoint(water) + 1);
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	SUBCASE("Make Area")
	{
		CHECK(space.m_sizeX == 10);
		CHECK(space.m_sizeY == 10);
		CHECK(space.m_sizeZ == 10);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		CHECK(FluidType::getName(water) == "water");
		CHECK(space.solid_get(Point3D::create(5, 5, 0)) == marble);
		area.doStep();
	}
	SUBCASE("Test move with threading")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin = Point3D::create(1, 1, 1);
		Point3D destination = Point3D::create(8, 8, 1);
		ActorIndex actor = actors.create(ActorParamaters{
			.species=dwarf,
			.location=origin,
		});
		actors.move_setDestination(actor, destination);
		area.doStep();
		++simulation.m_step;
		CHECK(actors.move_getPath(actor).size() == 7);
		CHECK(simulation.m_threadedTaskEngine.m_tasksForNextStep.empty());
		CHECK(actors.move_hasEvent(actor));
		Step scheduledStep = simulation.m_step + actors.move_stepsTillNextMoveEvent(actor) - 1;
		float seconds = (float)(actors.move_stepsTillNextMoveEvent(actor)).get() / (float)Config::stepsPerSecond.get();
		CHECK(seconds > 0.5f);
		CHECK(seconds < 0.9f);
		while(simulation.m_step != scheduledStep)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(actors.getLocation(actor) == origin);
		area.doStep();
		simulation.m_eventSchedule.doStep(simulation.m_step);
		CHECK(actors.getLocation(actor) != origin);
		while(actors.getLocation(actor) != destination)
		{
			area.doStep();
			simulation.m_eventSchedule.doStep(simulation.m_step);
			++simulation.m_step;
		}
		CHECK(actors.move_getDestination(actor).empty());
		CHECK(!actors.move_hasEvent(actor));
	}
}
TEST_CASE("vision-threading")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation("", Step::create(100000000000));
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Actors& actors = area.getActors();
	Point3D point1 = Point3D::create(3, 3, 1);
	Point3D point2 = Point3D::create(7, 7, 1);
	CHECK(point2.isInFrontOf(point1, point1.getFacingTwords(point2)));
	CHECK(point1.isInFrontOf(point2, point2.getFacingTwords(point1)));
	ActorIndex a1 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=point1,
		.facing=point1.getFacingTwords(point2),
	});
	CHECK(area.m_visionRequests.size() == 1);
	ActorIndex a2 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=point2,
		.facing=point2.getFacingTwords(point1),
	});
	CHECK(area.m_visionRequests.size() == 2);
	simulation.doStep();
	CHECK(actors.vision_canSeeActor(a1, a2));
	CHECK(actors.vision_canSeeActor(a2, a1));
}