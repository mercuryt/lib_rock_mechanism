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
	std::unordered_set<Item*> m_equipments;
	std::set<Item*, EquipmentSortByLayer> m_wearable;
	uint32_t m_mass;
public:
	EquipmentSet() : m_mass(0) { }
	void addEquipment(Item& equipment);
	void removeEquipment(Item& equipment);
	void modifyImpact(Hit& hit, const MaterialType& materialType, const BodyPartType& bodyPartType);
	std::vector<Attack> getAttacks();
	float getAttackCoolDownDurationModifier() const;
	bool contains(Item& item) const { return m_equipments.contains(&item); }
	const uint32_t& getMass() const;
};
