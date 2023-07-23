#include "equipment.h"
#include "config.h"
#include "randomUtil.h"
#include "hit.h"
#include "materialType.h"
#include "item.h"
#include "weaponType.h"

bool EquipmentSortByLayer::operator()(Item* const& a, Item* const& b) const
{
	assert(a->m_itemType.wearableType != nullptr);
	assert(b->m_itemType.wearableType != nullptr);
	return a->m_itemType.wearableType->layer > b->m_itemType.wearableType->layer;
}

void EquipmentSet::addEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) == m_equipments.end());
	//TODO: Allow tools to be equiped?
	assert(equipment.m_itemType.wearableType != nullptr || equipment.m_itemType.weaponType != nullptr);
	m_mass += equipment.m_mass;
	m_equipments.insert(&equipment);
	if(equipment.m_itemType.wearableType != nullptr)
		m_wearable.insert(&equipment);
}
void EquipmentSet::removeEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) != m_equipments.end());
	assert(m_mass >= equipment.m_mass);
	m_mass -= equipment.m_mass;
	m_equipments.erase(&equipment);
	if(equipment.m_itemType.wearableType != nullptr)
		m_wearable.erase(&equipment);
}
void EquipmentSet::modifyImpact(Hit& hit, const MaterialType& hitMaterialType, const BodyPartType& bodyPartType)
{
	// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
	for(Item* equipment : m_wearable)
	{
		auto& wearableType = *equipment->m_itemType.wearableType;
		if(std::ranges::find(wearableType.bodyPartsCovered, &bodyPartType) != wearableType.bodyPartsCovered.end() && randomUtil::percentChance(wearableType.percentCoverage))
		{
			uint32_t pierceScore = (hit.force / hit.area) * hitMaterialType.hardness * Config::pierceModifier;
			uint32_t defenseScore = wearableType.defenseScore * equipment->m_materialType.hardness;
			if(pierceScore > defenseScore)
			{
				hit.area = wearableType.rigid ? Config::convertBodyPartVolumeToArea(bodyPartType.volume) : equipment->m_itemType.wearableType->impactSpreadArea;
				hit.force -= wearableType.forceAbsorbedUnpiercedModifier * equipment->m_materialType.hardness * Config::forceAbsorbedUnpiercedModifier;
			}
			else
			{
				//TODO: Add wear to equipment.
				hit.force -= wearableType.forceAbsorbedPiercedModifier * equipment->m_materialType.hardness * Config::forceAbsorbedPiercedModifier;
			}

		}
	}
	std::erase_if(m_equipments, [](Item* equipment){ return equipment->m_percentWear == 100; });
}
std::vector<Attack> EquipmentSet::getAttacks()
{
	std::vector<Attack> output;
	for(Item* equipment : m_equipments)
		if(equipment->m_itemType.weaponType != nullptr)
			for(const AttackType& attackType : equipment->m_itemType.weaponType->attackTypes)
				output.emplace_back(&attackType, &equipment->m_materialType, equipment);
	return output;
}
const uint32_t& EquipmentSet::getMass() const
{
	return m_mass;
}
