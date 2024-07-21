#pragma once

#include "config.h"
#include "reference.h"
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

class EquipmentSet
{
	// TODO: Make this a vector.
	std::unordered_set<const BodyPartType*> m_bodyPartTypesWithRigidArmor;
	ItemReferences m_equipments;
	ItemReferencesMaybeSorted m_wearable;
	ItemReferences m_meleeWeapons;
	ItemReferences m_rangedWeapons;
	ItemReferences m_rangedWeaponAmmo;
	uint32_t m_mass = 0;
public:
	EquipmentSet(Area& area, const Json& data);
	void addEquipment(Area& area, ItemIndex equipment);
	void removeEquipment(Area& area, ItemIndex equipment);
	void modifyImpact(Area& area, Hit& hit, const BodyPartType& bodyPartType);
	void addGeneric(Area& area, const ItemType& itemType, const MaterialType& MaterialType, Quantity quantity);
	void removeGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void updateItemIndex(ItemIndex oldIndex, ItemIndex newIndex);
	void updateCarrierIndexForContents(Area& area, ItemIndex newIndex);
	std::vector<Attack> getMeleeAttacks(Area& area);
	auto& getRangedWeapons() { return m_rangedWeapons; }
	[[nodiscard]] bool contains(ItemIndex item) const;
	[[nodiscard]] bool containsItemType(const Area& area, const ItemType& itemType) const;
	[[nodiscard]] bool hasWeapons() const { return !m_meleeWeapons.empty() || !m_rangedWeapons.empty(); }
	[[nodiscard]] Step getLongestMeleeWeaponCoolDown(Area& area) const;
	[[nodiscard]] const uint32_t& getMass() const { return m_mass; }
	[[nodiscard]] bool canEquipCurrently(const Area& area, ActorIndex actor, ItemIndex item) const;
	[[nodiscard]] bool empty() const { return m_equipments.empty(); }
	[[nodiscard]] ItemIndex getWeaponToAttackAtRange(Area& area, float range);
	[[nodiscard]] ItemIndex getAmmoForRangedWeapon(Area& area, ItemIndex weapon);
	[[nodiscard]] auto& getAll() { return m_equipments; }
	[[nodiscard]] bool hasAnyEquipmentWithReservations(Area& area, ActorIndex actor) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(EquipmentSet, m_equipments);
};
