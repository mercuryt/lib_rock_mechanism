#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/item.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/materialType.h"
#include "animalSpecies.h"
#include "fight.h"
#include "grow.h"
#include <functional>
TEST_CASE("combat")
{
	const MaterialType& marble = MaterialType::byName("marble");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.addFaction(faction);
	Actor& dwarf1 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.m_blocks[1][1][1]);
	dwarf1.setFaction(&faction);
	area.m_hasActors.add(dwarf1);
	Item& pants = simulation.createItem(ItemType::byName("pants"), MaterialType::byName("plant matter"), 50u, 10);
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
		Item& longsword = simulation.createItem(ItemType::byName("long sword"), MaterialType::byName("bronze"), 50u, 10);
		dwarf1.m_equipmentSet.addEquipment(longsword);
		Item& pants = simulation.createItem(ItemType::byName("pants"), MaterialType::byName("plant matter"), 50u, 10);
		dwarf1.m_equipmentSet.addEquipment(pants);
		REQUIRE(dwarf1.m_canFight.getCombatScore() > initalScore);
		Actor& rabbit = simulation.createActor(AnimalSpecies::byName("dwarf rabbit"), area.m_blocks[2][2][1]);
		REQUIRE(rabbit.m_canFight.getCombatScore() < dwarf1.m_canFight.getCombatScore());
		dwarf1.m_canFight.setTarget(rabbit);
		REQUIRE(dwarf1.m_canFight.inRange(rabbit));
		REQUIRE(!rabbit.m_body.isInjured());
		dwarf1.m_canFight.attackMeleeRange(rabbit);
		REQUIRE(rabbit.m_body.isInjured());
	}
	SUBCASE("path to rabbit")
	{
		Actor& rabbit = simulation.createActor(AnimalSpecies::byName("dwarf rabbit"), area.m_blocks[5][5][1]);
		dwarf1.m_canFight.setTarget(rabbit);
		REQUIRE(dwarf1.m_canFight.hasThreadedTask());
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		REQUIRE(rabbit.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
	}
	SUBCASE("adjacent allies boost combat score")
	{
		uint32_t initalScore = dwarf1.m_canFight.getCurrentMeleeCombatScore();
		Actor& dwarf2 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.m_blocks[2][1][1]);
		dwarf2.setFaction(&faction);
		area.m_hasActors.add(dwarf2);
		REQUIRE(dwarf1.m_canFight.getCurrentMeleeCombatScore() > initalScore);
	}
	SUBCASE("flanking enemies reduce combat score")
	{
		uint32_t initalScore = dwarf1.m_canFight.getCurrentMeleeCombatScore();
		simulation.createActor(AnimalSpecies::byName("dwarf rabbit"), area.m_blocks[2][1][1]);
		simulation.createActor(AnimalSpecies::byName("dwarf rabbit"), area.m_blocks[1][2][1]);
		REQUIRE(dwarf1.m_canFight.getCurrentMeleeCombatScore() < initalScore);
	}
	SUBCASE("shoot rabbit")
	{
		Item& crossbow = simulation.createItem(ItemType::byName("crossbow"), MaterialType::byName("poplar wood"), 50u, 10u);
		dwarf1.m_equipmentSet.addEquipment(crossbow);
		Item& ammo = simulation.createItem(ItemType::byName("crossbow bolt"), MaterialType::byName("bronze"), 1u);
		dwarf1.m_equipmentSet.addEquipment(ammo);
		Actor& rabbit = simulation.createActor(AnimalSpecies::byName("dwarf rabbit"), area.m_blocks[3][3][1]);
		dwarf1.m_canFight.setTarget(rabbit);
		REQUIRE(dwarf1.m_canFight.inRange(rabbit));
		AttackType& attackType = dwarf1.m_canFight.getRangedAttackType(crossbow);
		Attack attack(&attackType, &ammo.m_materialType, &crossbow);
		REQUIRE(dwarf1.m_canFight.chanceOfProjectileHit(attack, rabbit) >= 100);
		REQUIRE(!rabbit.m_body.isInjured());
		dwarf1.m_canFight.attackLongRange(rabbit, &crossbow, &ammo);
		REQUIRE(rabbit.m_body.isInjured());
	}
}
