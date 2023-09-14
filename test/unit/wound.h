#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
TEST_CASE("wound")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& actor = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
	SUBCASE("bleed to death")
	{
		// Area, force, material type, wound type
		Hit hit(1,50, MaterialType::byName("poplar wood"), WoundType::Pierce);
		BodyPart& bodyPart = actor.m_body.pickABodyPartByVolume();
		actor.takeHit(hit, bodyPart);
		REQUIRE(hit.depth == 3);
		REQUIRE(bodyPart.wounds.size() == 1);
		REQUIRE(actor.m_body.isInjured());
		Wound& wound = bodyPart.wounds.front();
		REQUIRE(wound.bleedVolumeRate != 0);
		REQUIRE(actor.m_body.getStepsTillWoundsClose() > actor.m_body.getStepsTillBleedToDeath() + simulation.m_step);
		simulation.fastForward(actor.m_body.getStepsTillBleedToDeath());
		REQUIRE(!actor.m_alive);
		REQUIRE(actor.m_causeOfDeath == CauseOfDeath::bloodLoss);
	}
	SUBCASE("heal")
	{
		// Area, force, material type, wound type
		Hit hit(1,10, MaterialType::byName("poplar wood"), WoundType::Pierce);
		BodyPart& bodyPart = actor.m_body.pickABodyPartByVolume();
		actor.takeHit(hit, bodyPart);
		REQUIRE(hit.depth == 1);
		REQUIRE(bodyPart.wounds.size() == 1);
		REQUIRE(actor.m_body.isInjured());
		Wound& wound = bodyPart.wounds.front();
		REQUIRE(wound.bleedVolumeRate != 0);
		REQUIRE(actor.m_body.getStepsTillWoundsClose() < actor.m_body.getStepsTillBleedToDeath() + simulation.m_step);
		simulation.fastForward(actor.m_body.getStepsTillBleedToDeath());
		REQUIRE(actor.m_alive);
		REQUIRE(bodyPart.wounds.empty());
	}
}
