#pragma once

#include "config.h"
#include "reference.h"
#include "numericTypes/types.h"
#include "dataStructures/smallSet.h"

struct Attack;
struct Hit;
struct ItemType;
struct MaterialType;
class BodyPartType;
class Area;
class Items;

#include <vector>
#include <queue>
#include <set>

class EquipmentSet
{
	SmallSet<BodyPartTypeId> m_bodyPartTypesWithRigidArmor;
	SmallSet<ItemReference> m_equipments;
	SmallSet<ItemReference> m_wearable;
	SmallSet<ItemReference> m_meleeWeapons;
	SmallSet<ItemReference> m_rangedWeapons;
	SmallSet<ItemReference> m_rangedWeaponAmmo;
	Mass m_mass = Mass::create(0);
	bool m_wearableIsSorted;
public:
	EquipmentSet(Area& area, const Json& data);
	EquipmentSet() = default;
	void addEquipment(Area& area, const ItemIndex& equipment);
	void removeEquipment(Area& area, const ItemIndex& equipment);
	void modifyImpact(Area& area, Hit& hit, const BodyPartTypeId& bodyPartType);
	void addGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& MaterialType, const Quantity& quantity);
	void removeGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity);
	void updateCarrierIndexForContents(Area& area, const ItemIndex& newIndex);
	std::vector<Attack> getMeleeAttacks(Area& area);
	auto& getRangedWeapons() { return m_rangedWeapons; }
	[[nodiscard]] bool contains(const ItemReference& item) const;
	[[nodiscard]] bool containsItemType(const Area& area, const ItemTypeId& itemType) const;
	[[nodiscard]] bool hasWeapons() const { return !m_meleeWeapons.empty() || !m_rangedWeapons.empty(); }
	[[nodiscard]] Step getLongestMeleeWeaponCoolDown(Area& area) const;
	[[nodiscard]] const Mass& getMass() const { return m_mass; }
	// Area is not const here due to reference counting.
	[[nodiscard]] bool canEquipCurrently(Area& area, const ActorIndex& actor, const ItemIndex& item) const;
	[[nodiscard]] bool empty() const { return m_equipments.empty(); }
	[[nodiscard]] ItemIndex getWeaponToAttackAtRange(Area& area, const DistanceInBlocksFractional& range);
	[[nodiscard]] ItemIndex getAmmoForRangedWeapon(Area& area, const ItemIndex& weapon);
	[[nodiscard]] auto& getAll() { return m_equipments; }
	[[nodiscard]] auto& getAll() const { return m_equipments; }
	[[nodiscard]] bool hasAnyEquipmentWithReservations(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] ItemIndex getFirstItemWithType(const Area& area, const ItemTypeId& type) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(EquipmentSet, m_equipments);
};