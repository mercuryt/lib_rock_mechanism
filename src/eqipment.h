#pragma once

#include "fight.h"
#include "hit.h"
#include "materialType.h"
#include "body.h"
#include "item.h"

#include <vector>
#include <queue>
#include <unordered_set>

class EquipmentSortByLayer
{
	bool operator()(Item*& a, Item*& b);
};
class EquipmentSet
{
	std::unordered_set<Item*> m_equipments;
	std::priority_queue<Item*, std::vector<Item*>, EquipmentSortByLayer> m_wearable;
	uint32_t m_mass;
	EquipmentSet();
	void addEquipment(Item& equipment);
	void removeEquipment(Item& equipment);
	void modifyImpact(Hit& hit, const MaterialType& materialType, const BodyPartType& bodyPartType);
	std::vector<Attack> getAttacks();
};
