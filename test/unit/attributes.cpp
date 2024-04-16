#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/attributes.h"
#include "../../engine/animalSpecies.h"
#include "actor.h"
#include "areaBuilderUtil.h"
#include "config.h"
#include "simulation.h"

TEST_CASE("attributes")
{
		const AnimalSpecies& goblin = AnimalSpecies::byName("goblin");
	SUBCASE("basic")
	{
		Attributes attributes(goblin, 100);
		REQUIRE(attributes.getStrength() == goblin.strength[1]);
	}
	SUBCASE("move speed")
	{
		Simulation simulation;
		Area& area = simulation.createArea(10,10,10);
		areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("marble"));
		Actor& actor = simulation.createActor(ActorParamaters{
			.species = goblin,
			.location = &area.getBlock(5,5,1),
		});
		Block& adjacent = area.getBlock(5,6,1);
		actor.m_canMove.setDestination(adjacent);
		REQUIRE(actor.m_canMove.hasThreadedTask());
		simulation.doStep();
		REQUIRE(actor.m_canMove.hasEvent());
		float seconds = (float)actor.m_canMove.stepsTillNextMoveEvent() / (float)Config::stepsPerSecond;
		REQUIRE(seconds >= 0.2f);
		REQUIRE(seconds <= 0.3f);

	}
}
