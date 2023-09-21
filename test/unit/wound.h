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
	Block& pondLocation = area.m_blocks[3][7][1];
	pondLocation.setNotSolid();
	pondLocation.addFluid(Config::maxBlockVolume, FluidType::byName("water"));
	Item& fruit = simulation.createItem(ItemType::byName("apple"), MaterialType::byName("fruit"), 50u);
	Block& fruitLocation = area.m_blocks[6][5][2];
	fruit.setLocation(fruitLocation);
	SUBCASE("bleed to death")
	{
		// Area, force, material type, wound type
		Hit hit(1,30, MaterialType::byName("poplar wood"), WoundType::Pierce);
		BodyPart& bodyPart = actor.m_body.pickABodyPartByVolume();
		actor.takeHit(hit, bodyPart);
		REQUIRE(hit.depth == 3);
		REQUIRE(bodyPart.wounds.size() == 1);
		REQUIRE(actor.m_body.isInjured());
		Wound& wound = bodyPart.wounds.front();
		REQUIRE(wound.bleedVolumeRate != 0);
		REQUIRE(actor.m_body.getStepsTillWoundsClose() > actor.m_body.getStepsTillBleedToDeath());
		simulation.fastForward(actor.m_body.getStepsTillBleedToDeath());
		REQUIRE(!actor.m_alive);
		REQUIRE(actor.m_causeOfDeath == CauseOfDeath::bloodLoss);
	}
	SUBCASE("heal")
	{
		// Area, force, material type, wound type
		Hit hit(1,11, MaterialType::byName("poplar wood"), WoundType::Pierce);
		BodyPart& bodyPart = actor.m_body.pickABodyPartByVolume();
		actor.takeHit(hit, bodyPart);
		REQUIRE(hit.depth == 1);
		REQUIRE(bodyPart.wounds.size() == 1);
		REQUIRE(actor.m_body.isInjured());
		Wound& wound = bodyPart.wounds.front();
		REQUIRE(wound.bleedVolumeRate != 0);
		REQUIRE(actor.m_body.getStepsTillWoundsClose() < actor.m_body.getStepsTillBleedToDeath());
		simulation.fastForward(actor.m_body.getStepsTillWoundsClose());
		REQUIRE(actor.m_alive);
		REQUIRE(!actor.m_body.hasBleedEvent());
		REQUIRE(wound.bleedVolumeRate == 0);
		simulation.fastForward(wound.healEvent.remainingSteps());
		REQUIRE(bodyPart.wounds.empty());
	}
	SUBCASE("impairment")
	{
		BodyPart& leftLeg = actor.m_body.pickABodyPartByType(BodyPartType::byName("left leg"));
		BodyPart& leftArm = actor.m_body.pickABodyPartByType(BodyPartType::byName("left arm"));
		Hit hit(1,5, MaterialType::byName("bronze"), WoundType::Pierce);
		auto moveSpeed = actor.m_canMove.getMoveSpeed();
		actor.takeHit(hit, leftLeg);
		REQUIRE(actor.m_body.getImpairMovePercent() != 0);
		REQUIRE(moveSpeed > actor.m_canMove.getMoveSpeed());
		auto combatScore = actor.m_canFight.getCombatScore();
		REQUIRE(combatScore != 0);
		Hit hit2(1,5, MaterialType::byName("bronze"), WoundType::Pierce);
		actor.takeHit(hit2, leftArm);
		REQUIRE(actor.m_body.getImpairManipulationPercent() != 0);
		REQUIRE(combatScore > actor.m_canFight.getCombatScore());
	}
	//TODO
	/*SUBCASE("sever")
	{
		// Area, force, material type, wound type
		Hit hit(20,50, MaterialType::byName("bronze"), WoundType::Cut);
		BodyPart& leftArm = actor.m_body.pickABodyPartByType(BodyPartType::byName("left arm"));
		actor.takeHit(hit, leftArm);
		REQUIRE(actor.m_body.getImpairPercentFor(BodyPartType::byName("left arm")) == 100u);
		REQUIRE(actor.m_body.hasBleedEvent());
	}
	*/
	SUBCASE("fatal blow")
	{
		// Area, force, material type, wound type
		Hit hit(10,400, MaterialType::byName("bronze"), WoundType::Cut);
		BodyPart& head = actor.m_body.pickABodyPartByType(BodyPartType::byName("head"));
		actor.takeHit(hit, head);
		REQUIRE(!actor.m_alive);
		REQUIRE(actor.m_causeOfDeath == CauseOfDeath::wound);
	}
}
