#pragma once

#include <vector>
#include <list>
#include <queue>
#include <unordered_multimap>

template<class EquipmentType,  class MaterialType>
class Equipment
{
	const EquipmentType& m_equipmentType;
	const MaterialType& m_materialType;
	uint32_t m_percentWear;
	uint32_t m_quality;
	bool pierced(uint32_t area)
	{
		if(area >= m_equipmentType.area)
			m_percentWear = 100;
		else
			m_percentWear += (area / m_equipmentType.area) * 100u;
	}
	bool operator==(Equipment& equipment){ return &equipment == this; }
}

static std::list<Equipment> s_allEquipment;

class EquipmentSortByLayer
{
	bool operator()(Equipment*& a, Equipment*& b)
	{
		return a->m_equipmentType < b->m_equipmentType;
	}
}

template<class EquipmentType, class Equipment, class BodyPartType>
class EquipmentSet
{
	std::priority_queue<Equipment*, std::vector<Equipment*, EquipmentSortByLayer> m_equipments;
	std::unordered_multimap<uint32_t, Equipment*> m_attackTable;
	uint32_t m_mass;

	EquipmentSet() : m_mass(0) {}

	void addEquipment(Equipment& equipment)
	{
		assert(std::ranges::find(m_equipments, &equipment) != m_equipments.end());
		m_mass += equipment.m_equipmentType.mass;
		m_equipments.insert(&equipment);
	}
	void removeEquipment(Equipment& equipment)
	{
		assert(std::ranges::find(m_equipments, &equipment) != m_equipments.end());
		assert(m_mass >= equipment.m_equipmentType.mass);
		m_mass -= equipment.m_equipmentType.mass;
		std::erase(m_equipments, &equipment);
	}
	void modifyImpact(Hit& hit, const MaterialType& materialType, const BodyPartType& bodyPartType)
	{
		// Priority queue is sorted by layers so we start at the outside and pierce inwards.
		for(Equipment* equipment : m_equipments)
			if(equipment->m_equipmentType.bodyPartsCovered.contains(&BodyPartType) && randomUtil::percentChance(equipment->m_equipmentType.percentCoverage))
			{
				uint32_t pierceScore = (hit.force / hit.area) * materialType.hardness * Config::pierceModifier;
				uint32_t defenseScore = equipment.m_equipmentType.defenseScore * equipment.m_materialType.hardness;
				if(pierceScore > defenseScore)
				{
					hit.area = equipment.m_equipmentType.rigid ? bodyPartType.area : equipment.m_equipmentType.impactSpreadArea;
					hit.force -= equipment.m_equipmentType.forceAbsorbedUnpiercedModifier * equipment.materialType.hardness * Config::forceAbsorbedUnpiercedModifier;
				}
				else
				{
					equipment.pierced(hit.area);
					hit.force -= equipment.m_equipmentType.forceAbsorbedPiercedModifier * equipment.materialType.hardness * Config::forceAbsorbedPiercedModifier;
				}

			}
		std::erase_if(m_equipments, [](Equipment& equipment){ return equipment.m_percentWear == 100; });
	}
}
