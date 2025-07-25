#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/definitions/bodyType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/definitions/itemType.h"
TEST_CASE("wound")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	ActorIndex actor = actors.create({
		.species=dwarf,
		.location=Point3D::create(1, 1, 2),
	});
	Point3D pondLocation = Point3D::create(3, 7, 1);
	space.solid_setNot(pondLocation);
	space.fluid_add(pondLocation, Config::maxPointVolume, FluidType::byName("water"));
	Point3D fruitLocation = Point3D::create(6, 5, 2);
	items.create({.itemType=ItemType::byName("apple"), .materialType=MaterialType::byName("fruit"), .location=fruitLocation, .quantity=Quantity::create(50u)});
	SUBCASE("bleed to death")
	{
		// Area, force, material type, wound type
		Hit hit(1, Force::create(35), MaterialType::byName("poplar wood"), WoundType::Pierce);
		BodyPart& bodyPart = actors.body_pickABodyPartByVolume(actor);
		actors.takeHit(actor, hit, bodyPart);
		CHECK(hit.depth == 3);
		CHECK(bodyPart.wounds.size() == 1);
		CHECK(actors.body_isInjured(actor));
		Wound& wound = bodyPart.wounds.front();
		CHECK(wound.bleedVolumeRate != 0);
		CHECK(actors.body_getStepsTillWoundsClose(actor) > actors.body_getStepsTillBleedToDeath(actor));
		simulation.fastForward(actors.body_getStepsTillBleedToDeath(actor));
		CHECK(!actors.isAlive(actor));
		CHECK(actors.getCauseOfDeath(actor) == CauseOfDeath::bloodLoss);
	}
	SUBCASE("heal")
	{
		// Area, force, material type, wound type
		Hit hit(1, Force::create(11), MaterialType::byName("poplar wood"), WoundType::Pierce);
		BodyPart& bodyPart = actors.body_pickABodyPartByVolume(actor);
		actors.takeHit(actor, hit, bodyPart);
		CHECK(hit.depth == 1);
		CHECK(bodyPart.wounds.size() == 1);
		CHECK(actors.body_isInjured(actor));
		Wound& wound = bodyPart.wounds.front();
		CHECK(wound.bleedVolumeRate != 0);
		CHECK(actors.body_getStepsTillWoundsClose(actor) < actors.body_getStepsTillBleedToDeath(actor));
		simulation.fastForward(actors.body_getStepsTillWoundsClose(actor));
		CHECK(actors.isAlive(actor));
		CHECK(!actors.body_hasBleedEvent(actor));
		CHECK(wound.bleedVolumeRate == 0);
		simulation.fastForward(wound.healEvent.remainingSteps());
		CHECK(bodyPart.wounds.empty());
	}
	SUBCASE("impairment")
	{
		BodyPart& leftLeg = actors.body_pickABodyPartByType(actor, BodyPartType::byName("left leg"));
		BodyPart& leftArm = actors.body_pickABodyPartByType(actor, BodyPartType::byName("left arm"));
		Hit hit(1, Force::create(5), MaterialType::byName("bronze"), WoundType::Pierce);
		auto moveSpeed = actors.move_getSpeed(actor);
		actors.takeHit(actor, hit, leftLeg);
		CHECK(actors.body_getImpairMovePercent(actor) != 0);
		CHECK(moveSpeed > actors.move_getSpeed(actor));
		auto combatScore = actors.combat_getCombatScore(actor);
		CHECK(combatScore != 0);
		Hit hit2(1, Force::create(5), MaterialType::byName("bronze"), WoundType::Pierce);
		actors.takeHit(actor, hit2, leftArm);
		CHECK(actors.body_getImpairManipulationPercent(actor) != 0);
		CHECK(combatScore > actors.combat_getCombatScore(actor));
	}
	//TODO
	/*SUBCASE("sever")
	{
		// Area, force, material type, wound type
		Hit hit(20,50, MaterialType::byName("bronze"), WoundType::Cut);
		BodyPart& leftArm = actor.m_body.pickABodyPartByType(BodyPartType::byName("left arm"));
		actor.takeHit(hit, leftArm);
		CHECK(actor.m_body.getImpairPercentFor(BodyPartType::byName("left arm")) == 100u);
		CHECK(actors.body_hasBleedEvent(actor));
	}
	*/
	SUBCASE("fatal blow")
	{
		// Area, force, material type, wound type
		Hit hit(10, Force::create(400), MaterialType::byName("bronze"), WoundType::Cut);
		BodyPart& head = actors.body_pickABodyPartByType(actor, BodyPartType::byName("head"));
		actors.takeHit(actor, hit, head);
		CHECK(!actors.isAlive(actor));
		CHECK(actors.getCauseOfDeath(actor) == CauseOfDeath::wound);
	}
}
