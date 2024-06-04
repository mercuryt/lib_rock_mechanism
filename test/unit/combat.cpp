#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/materialType.h"
#include "../../engine/animalSpecies.h"
#include "types.h"
#include <functional>
TEST_CASE("combat")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.registerFaction(faction);
	Actor& dwarf1 = simulation.m_hasActors->createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"),
		.location=blocks.getIndex({1, 1, 1}),
		.area=&area,
		.hasCloths=false,
		.hasSidearm=false,
	});
	dwarf1.setFaction(&faction);
	Item& pants = simulation.m_hasItems->createItemNongeneric(ItemType::byName("pants"), MaterialType::byName("plant matter"), 50u, 10);
	dwarf1.m_equipmentSet.addEquipment(pants);
	SUBCASE("attack table")
	{
		auto& attackTable = dwarf1.m_canFight.getAttackTable();
		REQUIRE(attackTable.size() == dwarf1.m_body.getMeleeAttacks().size());
		REQUIRE(attackTable.size() == 4);
		REQUIRE(dwarf1.m_canFight.getCombatScore() == 26);
		REQUIRE(dwarf1.m_canFight.getMaxRange() == 1.5f);
		REQUIRE(dwarf1.m_canFight.getCoolDownDurationModifier() == 1.f);
	}
	SUBCASE("strike rabbit")
	{
		uint32_t initalScore = dwarf1.m_canFight.getCombatScore();
		Item& longsword = simulation.m_hasItems->createItemNongeneric(ItemType::byName("long sword"), MaterialType::byName("bronze"), 50u, 10);
		dwarf1.m_equipmentSet.addEquipment(longsword);
		Item& pants = simulation.m_hasItems->createItemNongeneric(ItemType::byName("pants"), MaterialType::byName("plant matter"), 50u, 10);
		dwarf1.m_equipmentSet.addEquipment(pants);
		REQUIRE(dwarf1.m_canFight.getCombatScore() > initalScore);
		Actor& rabbit = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf rabbit"), blocks.getIndex({2, 2, 1}));
		REQUIRE(rabbit.m_canFight.getCombatScore() < dwarf1.m_canFight.getCombatScore());
		dwarf1.m_canFight.setTarget(rabbit);
		REQUIRE(dwarf1.m_canFight.inRange(rabbit));
		REQUIRE(!rabbit.m_body.isInjured());
		dwarf1.m_canFight.attackMeleeRange(rabbit);
		REQUIRE(rabbit.m_body.isInjured());
	}
	SUBCASE("path to rabbit")
	{
		Actor& rabbit = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf rabbit"), blocks.getIndex({5, 5, 1}));
		dwarf1.m_canFight.setTarget(rabbit);
		REQUIRE(dwarf1.m_canFight.hasThreadedTask());
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		REQUIRE(rabbit.isAdjacentTo(dwarf1.m_canMove.getDestination()));
	}
	SUBCASE("adjacent allies boost combat score")
	{
		uint32_t initalScore = dwarf1.m_canFight.getCurrentMeleeCombatScore();
		Actor& dwarf2 = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf"), blocks.getIndex({2, 1, 1}));
		dwarf2.setFaction(&faction);
		REQUIRE(dwarf1.m_canFight.getCurrentMeleeCombatScore() > initalScore);
	}
	SUBCASE("flanking enemies reduce combat score")
	{
		uint32_t initalScore = dwarf1.m_canFight.getCurrentMeleeCombatScore();
		simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf rabbit"), blocks.getIndex({2, 1, 1}));
		simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf rabbit"), blocks.getIndex({1, 2, 1}));
		REQUIRE(dwarf1.m_canFight.getCurrentMeleeCombatScore() < initalScore);
	}
	SUBCASE("shoot rabbit")
	{
		Item& crossbow = simulation.m_hasItems->createItemNongeneric(ItemType::byName("crossbow"), MaterialType::byName("poplar wood"), 50u, 10u);
		dwarf1.m_equipmentSet.addEquipment(crossbow);
		Item& ammo = simulation.m_hasItems->createItemGeneric(ItemType::byName("crossbow bolt"), MaterialType::byName("bronze"), 1u);
		dwarf1.m_equipmentSet.addEquipment(ammo);
		Actor& rabbit = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf rabbit"), blocks.getIndex({3, 3, 1}));
		dwarf1.m_canFight.setTarget(rabbit);
		REQUIRE(dwarf1.m_canFight.inRange(rabbit));
		AttackType& attackType = dwarf1.m_canFight.getRangedAttackType(crossbow);
		Attack attack(&attackType, &ammo.m_materialType, &crossbow);
		REQUIRE(dwarf1.m_canFight.projectileHitPercent(attack, rabbit) >= 100);
		REQUIRE(!rabbit.m_body.isInjured());
		dwarf1.m_canFight.attackLongRange(rabbit, &crossbow, &ammo);
		REQUIRE(rabbit.m_body.isInjured());
	}
}
