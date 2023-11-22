#pragma once

#include "fight.h"
#include "hit.h"
#include "materialType.h"
#include "body.h"
#include "item.h"

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
	Actor& m_actor;
	std::unordered_set<Item*> m_equipments;
	std::set<Item*, EquipmentSortByLayer> m_wearable;
	std::unordered_set<const BodyPartType*> m_bodyPartTypesWithRigidArmor;
	std::unordered_set<Item*> m_meleeWeapons;
	std::unordered_set<Item*> m_rangedWeapons;
	std::unordered_set<Item*> m_rangedWeaponAmmo;
	uint32_t m_mass;
public:
	EquipmentSet(Actor& a) : m_actor(a), m_mass(0) { }
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
};
