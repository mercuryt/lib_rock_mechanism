#pragma once

#include "config.h"

class Actor;
class Item;
class BodyPartType;
struct Attack;
struct Hit;

#include <vector>
#include <queue>
#include <unordered_set>
#include <set>

struct EquipmentSortByLayer
{
	bool operator()(Item* const& a, Item* const& b) const;
};
class EquipmentSet
{
	std::unordered_set<Item*> m_equipments;
	std::set<Item*, EquipmentSortByLayer> m_wearable;
	std::unordered_set<const BodyPartType*> m_bodyPartTypesWithRigidArmor;
	std::unordered_set<Item*> m_meleeWeapons;
	std::unordered_set<Item*> m_rangedWeapons;
	std::unordered_set<Item*> m_rangedWeaponAmmo;
	Actor& m_actor;
	uint32_t m_mass = 0;
	void insertEquipment(Item& equipment);
public:
	EquipmentSet(Actor& a) : m_actor(a) { }
	EquipmentSet(const Json& data, Actor& a);
	Json toJson() const;
	void addEquipment(Item& equipment);
	void removeEquipment(Item& equipment);
	void modifyImpact(Hit& hit, const BodyPartType& bodyPartType);
	std::vector<Attack> getMeleeAttacks();
	std::unordered_set<Item*>& getRangedWeapons() { return m_rangedWeapons; }
	[[nodiscard]] bool contains(Item& item) const { return m_equipments.contains(&item); }
	[[nodiscard]] bool hasWeapons() const;
	[[nodiscard]] Step getLongestMeleeWeaponCoolDown() const;
	[[nodiscard]] const uint32_t& getMass() const;
	[[nodiscard]] bool canEquipCurrently(Item& item) const;
	[[nodiscard]] bool empty() const { return m_equipments.empty(); }
	[[nodiscard]] Item* getWeaponToAttackAtRange(float range);
	[[nodiscard]] Item* getAmmoForRangedWeapon(Item& weapon);
	[[nodiscard]] std::unordered_set<Item*>& getAll() { return m_equipments; }
	[[nodiscard]] bool hasAnyEquipmentWithReservations() const;
};
