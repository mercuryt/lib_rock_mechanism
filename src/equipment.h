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
	uint32_t m_mass;
public:
	EquipmentSet(Actor& a) : m_actor(a), m_mass(0) { }
	void addEquipment(Item& equipment);
	void removeEquipment(Item& equipment);
	void modifyImpact(Hit& hit, const BodyPartType& bodyPartType);
	std::vector<Attack> getAttacks();
	[[nodiscard]]bool contains(Item& item) const { return m_equipments.contains(&item); }
	[[nodiscard]]const uint32_t& getMass() const;
	[[nodiscard]]bool canEquipCurrently(Item& item) const;
	[[nodiscard]]bool empty() const { return m_equipments.empty(); }
};
