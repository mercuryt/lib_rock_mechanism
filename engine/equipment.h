#pragma once

#include "config.h"
#include "reference.h"
#include "types.h"
#include "vectorContainers.h"

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
	ItemReferencesMaybeSorted m_wearable;
	ItemReferences m_equipments;
	ItemReferences m_meleeWeapons;
	ItemReferences m_rangedWeapons;
	ItemReferences m_rangedWeaponAmmo;
	Mass m_mass = Mass::create(0);
public:
	EquipmentSet(Area& area, const Json& data);
	EquipmentSet() = default;
	void addEquipment(Area& area, ItemIndex equipment);
	void removeEquipment(Area& area, ItemIndex equipment);
	void modifyImpact(Area& area, Hit& hit, BodyPartTypeId bodyPartType);
	void addGeneric(Area& area, ItemTypeId itemType, MaterialTypeId MaterialType, Quantity quantity);
	void removeGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void updateCarrierIndexForContents(Area& area, ItemIndex newIndex);
	std::vector<Attack> getMeleeAttacks(Area& area);
	auto& getRangedWeapons() { return m_rangedWeapons; }
	[[nodiscard]] bool contains(ItemIndex item) const;
	[[nodiscard]] bool containsItemType(const Area& area, ItemTypeId itemType) const;
	[[nodiscard]] bool hasWeapons() const { return !m_meleeWeapons.empty() || !m_rangedWeapons.empty(); }
	[[nodiscard]] Step getLongestMeleeWeaponCoolDown(Area& area) const;
	[[nodiscard]] const Mass& getMass() const { return m_mass; }
	[[nodiscard]] bool canEquipCurrently(const Area& area, ActorIndex actor, ItemIndex item) const;
	[[nodiscard]] bool empty() const { return m_equipments.empty(); }
	[[nodiscard]] ItemIndex getWeaponToAttackAtRange(Area& area, DistanceInBlocksFractional range);
	[[nodiscard]] ItemIndex getAmmoForRangedWeapon(Area& area, ItemIndex weapon);
	[[nodiscard]] auto& getAll() { return m_equipments; }
	[[nodiscard]] bool hasAnyEquipmentWithReservations(Area& area, ActorIndex actor) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(EquipmentSet, m_equipments);
};
