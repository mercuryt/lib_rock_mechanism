#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/numericTypes/types.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/hasShapes.hpp"
#include <functional>
TEST_CASE("combat")
{
	MaterialTypeId marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, DistanceInBlocks::create(0), marble);
	FactionId faction = simulation.createFaction("Tower Of Power");
	area.m_hasStockPiles.registerFaction(faction);
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName("dwarf"),
		.location=blocks.getIndex_i(1, 1, 1),
		.hasCloths=false,
		.hasSidearm=false,
	});
	actors.setFaction(dwarf1, faction);
	ItemIndex pants = items.create({
		.itemType=ItemType::byName("pants"),
		.materialType=MaterialType::byName("plant matter"),
		.quality=Quality::create(50u),
		.percentWear=Percent::create(10)
	});
	actors.equipment_add(dwarf1, pants);
	SUBCASE("attack table")
	{
		auto& attackTable = actors.combat_getAttackTable(dwarf1);
		CHECK(attackTable.size() == actors.combat_getMeleeAttacks(dwarf1).size());
		CHECK(attackTable.size() == 4);
		CHECK(actors.combat_getCombatScore(dwarf1) == 26);
		CHECK(actors.combat_getMaxRange(dwarf1) == 1.5f);
		CHECK(actors.combat_getCoolDownDurationModifier(dwarf1) == 1.f);
	}
	SUBCASE("strike rabbit")
	{
		CHECK(ItemType::getIsWeapon(ItemType::byName("long sword")));
		CombatScore initalScore = actors.combat_getCombatScore(dwarf1);
		CHECK(actors.combat_getAttackTable(dwarf1).size() == 4);
		ItemIndex longsword = items.create({
			.itemType=ItemType::byName("long sword"),
			.materialType=MaterialType::byName("bronze"),
			.quality=Quality::create(50u),
			.percentWear=Percent::create(10)
		});
		actors.equipment_add(dwarf1, longsword);
		CHECK(actors.combat_getAttackTable(dwarf1).size() == 7);
		ItemIndex pants = items.create({
			.itemType=ItemType::byName("pants"),
			.materialType=MaterialType::byName("plant matter"),
			.quality=Quality::create(50u),
			.percentWear=Percent::create(10),
		});
		actors.equipment_add(dwarf1, pants);
		CHECK(actors.combat_getCombatScore(dwarf1) > initalScore);
		ActorIndex rabbit = actors.create({
			.species=AnimalSpecies::byName("dwarf rabbit"),
			.location=blocks.getIndex_i(2, 2, 1),
		});
		CHECK(actors.combat_getCombatScore(rabbit) < actors.combat_getCombatScore(dwarf1));
		actors.combat_setTarget(dwarf1, rabbit);
		CHECK(actors.combat_inRange(dwarf1, rabbit));
		CHECK(!actors.body_isInjured(rabbit));
		actors.combat_attackMeleeRange(dwarf1, rabbit);
		CHECK(actors.body_isInjured(rabbit));
	}
	SUBCASE("path to rabbit")
	{
		ActorIndex rabbit = actors.create({
			.species=AnimalSpecies::byName("dwarf rabbit"),
			.location=blocks.getIndex_i(5, 5, 1),
		});
		actors.combat_setTarget(dwarf1, rabbit);
		CHECK(actors.move_hasPathRequest(dwarf1));
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		CHECK(actors.isAdjacentToLocation(rabbit, actors.move_getDestination(dwarf1)));
	}
	SUBCASE("adjacent allies boost combat score")
	{
		CombatScore initalScore = actors.combat_getCurrentMeleeCombatScore(dwarf1);
		actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=blocks.getIndex_i(2, 1, 1),
			.faction=faction
		});
		CHECK(actors.combat_getCurrentMeleeCombatScore(dwarf1) > initalScore);
	}
	SUBCASE("flanking enemies reduce combat score")
	{
		CombatScore initalScore = actors.combat_getCurrentMeleeCombatScore(dwarf1);
		actors.create({
			.species=AnimalSpecies::byName("dwarf rabbit"),
			.location=blocks.getIndex_i(2, 1, 1),
		});
		actors.create({
			.species=AnimalSpecies::byName("dwarf rabbit"),
			.location=blocks.getIndex_i(1, 2, 1),
		});
		CHECK(actors.combat_getCurrentMeleeCombatScore(dwarf1) < initalScore);
	}
	SUBCASE("shoot rabbit")
	{
		ItemIndex crossbow = items.create({
			.itemType=ItemType::byName("crossbow"),
			.materialType=MaterialType::byName("poplar wood"),
			.quality=Quality::create(50u),
			.percentWear=Percent::create(10u)
		});
		actors.equipment_add(dwarf1, crossbow);
		ItemIndex ammo = items.create({
			.itemType=ItemType::byName("crossbow bolt"),
			.materialType=MaterialType::byName("bronze"),
			.quantity=Quantity::create(1u)
		});
		actors.equipment_add(dwarf1, ammo);
		ActorIndex rabbit = actors.create({
			.species=AnimalSpecies::byName("dwarf rabbit"),
			.location=blocks.getIndex_i(3, 3, 1),
		});
		actors.combat_setTarget(dwarf1, rabbit);
		CHECK(actors.combat_inRange(dwarf1, rabbit));
		AttackTypeId attackType = actors.combat_getRangedAttackType(dwarf1, crossbow);
		Attack attack(attackType, items.getMaterialType(ammo), crossbow);
		CHECK(actors.combat_projectileHitPercent(dwarf1, attack, rabbit) >= 100);
		CHECK(!actors.body_isInjured(rabbit));
		actors.combat_attackLongRange(dwarf1, rabbit, crossbow, ammo);
		CHECK(actors.body_isInjured(rabbit));
	}
}
