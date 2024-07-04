#pragma once

#include "config.h"
#include "types.h"

struct Attack;
struct Hit;
struct ItemType;
struct MaterialType;
class BodyPartType;
class Area;
class Items;

#include <vector>
#include <queue>
#include <unordered_set>
#include <set>

struct EquipmentSortByLayer
{
	Items& items;
	bool operator()(ItemIndex const& a, ItemIndex const& b) const;
};
class EquipmentSet
{
	std::unordered_set<ItemIndex> m_equipments;
	std::set<ItemIndex, EquipmentSortByLayer> m_wearable;
	std::unordered_set<const BodyPartType*> m_bodyPartTypesWithRigidArmor;
	std::unordered_set<ItemIndex> m_meleeWeapons;
	std::unordered_set<ItemIndex> m_rangedWeapons;
	std::unordered_set<ItemIndex> m_rangedWeaponAmmo;
	uint32_t m_mass = 0;
public:
	EquipmentSet(const Json& data);
	Json toJson() const;
	void addEquipment(Area& area, ItemIndex equipment);
	void removeEquipment(Area& area, ItemIndex equipment);
	void modifyImpact(Area& area, Hit& hit, const BodyPartType& bodyPartType);
	void addGeneric(Area& area, const ItemType& itemType, const MaterialType& MaterialType, Quantity quantity);
	void removeGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	std::vector<Attack> getMeleeAttacks(Area& area);
	std::unordered_set<ItemIndex>& getRangedWeapons() { return m_rangedWeapons; }
	[[nodiscard]] bool contains(ItemIndex item) const { return m_equipments.contains(item); }
	[[nodiscard]] bool containsItemType(const Area& area, const ItemType& itemType) const;
	[[nodiscard]] bool hasWeapons() const { return !m_meleeWeapons.empty() || !m_rangedWeapons.empty(); }
	[[nodiscard]] Step getLongestMeleeWeaponCoolDown(Area& area) const;
	[[nodiscard]] const uint32_t& getMass() const { return m_mass; }
	[[nodiscard]] bool canEquipCurrently(const Area& area, ActorIndex actor, ItemIndex item) const;
	[[nodiscard]] bool empty() const { return m_equipments.empty(); }
	[[nodiscard]] ItemIndex getWeaponToAttackAtRange(Area& area, float range);
	[[nodiscard]] ItemIndex getAmmoForRangedWeapon(Area& area, ItemIndex weapon);
	[[nodiscard]] std::unordered_set<ItemIndex>& getAll() { return m_equipments; }
	[[nodiscard]] bool hasAnyEquipmentWithReservations(Area& area, ActorIndex actor) const;
};
