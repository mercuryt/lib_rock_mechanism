#include "../../lib/doctest.h"
#include "../../engine/attributes.h"
#include "../../engine/animalSpecies.h"
#include "config.h"

TEST_CASE("attributes")
{
	const AnimalSpecies& goblin = AnimalSpecies::byName("goblin");
	Attributes attributes(goblin, 100);
	REQUIRE(attributes.getStrength() == goblin.strength[1]);
	Step stepsToEnterBlock = (Config::stepsPerSecond * Config::baseMoveCost) / attributes.getMoveSpeed();
	float seconds = (float)stepsToEnterBlock / (float)Config::stepsPerSecond;
	REQUIRE(seconds <= 0.3f);
	REQUIRE(seconds >= 0.2f);
}
