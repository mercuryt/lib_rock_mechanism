#include "equipment.h"
#include "config.h"
#include "randomUtil.h"
#include "hit.h"
#include "materialType.h"
#include "item.h"
#include "weaponType.h"
#include "actor.h"

bool EquipmentSortByLayer::operator()(Item* const& a, Item* const& b) const
{
	assert(a->m_itemType.wearableData != nullptr);
	assert(b->m_itemType.wearableData != nullptr);
	return a->m_itemType.wearableData->layer > b->m_itemType.wearableData->layer;
}

void EquipmentSet::addEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) == m_equipments.end());
	m_mass += equipment.m_mass;
	m_equipments.insert(&equipment);
	if(equipment.m_itemType.wearableData != nullptr)
	{
		m_wearable.insert(&equipment);
		if(equipment.m_itemType.wearableData->rigid)
			for(const BodyPartType* bodyPartType : equipment.m_itemType.wearableData->bodyPartsCovered)
			{
				assert(!m_bodyPartTypesWithRigidArmor.contains(bodyPartType));
				m_bodyPartTypesWithRigidArmor.insert(bodyPartType);
			}
	}
}
void EquipmentSet::removeEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) != m_equipments.end());
	assert(m_mass >= equipment.m_mass);
	m_mass -= equipment.m_mass;
	m_equipments.erase(&equipment);
	if(equipment.m_itemType.wearableData != nullptr)
		m_wearable.erase(&equipment);
}
void EquipmentSet::modifyImpact(Hit& hit, const MaterialType& hitMaterialType, const BodyPartType& bodyPartType)
{
	// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
	for(Item* equipment : m_wearable)
	{
		auto& wearableData = *equipment->m_itemType.wearableData;
		if(std::ranges::find(wearableData.bodyPartsCovered, &bodyPartType) != wearableData.bodyPartsCovered.end() && randomUtil::percentChance(wearableData.percentCoverage))
		{
			uint32_t pierceScore = (hit.force / hit.area) * hitMaterialType.hardness * Config::pierceModifier;
			uint32_t defenseScore = wearableData.defenseScore * equipment->m_materialType.hardness;
			if(pierceScore < defenseScore)
			{
				if(wearableData.rigid)
					hit.area = Config::convertBodyPartVolumeToArea(bodyPartType.volume);
				hit.force -= wearableData.forceAbsorbedUnpiercedModifier * equipment->m_materialType.hardness * Config::forceAbsorbedUnpiercedModifier;
			}
			else
			{
				//TODO: Add wear to equipment.
				hit.force -= wearableData.forceAbsorbedPiercedModifier * equipment->m_materialType.hardness * Config::forceAbsorbedPiercedModifier;
			}

		}
	}
	std::erase_if(m_equipments, [](Item* equipment){ return equipment->m_percentWear == 100; });
}
std::vector<Attack> EquipmentSet::getAttacks()
{
	std::vector<Attack> output;
	for(Item* equipment : m_equipments)
		if(equipment->m_itemType.weaponData != nullptr)
			for(const AttackType& attackType : equipment->m_itemType.weaponData->attackTypes)
				output.emplace_back(&attackType, &equipment->m_materialType, equipment);
	return output;
}
const uint32_t& EquipmentSet::getMass() const
{
	return m_mass;
}
bool EquipmentSet::canEquipCurrently(Item& item) const
{
	assert(!m_equipments.contains(&item));
	if(item.m_itemType.wearableData != nullptr)
	{
		for(const BodyPartType* bodyPartType : item.m_itemType.wearableData->bodyPartsCovered)
			if(!m_actor.m_species.bodyType.hasBodyPart(*bodyPartType))
				return false;
		if(&item.m_itemType.wearableData->bodyTypeScale != &m_actor.m_species.bodyScale)
			return false;
		if(item.m_itemType.wearableData->rigid)
			for(const BodyPartType* bodyPartType : item.m_itemType.wearableData->bodyPartsCovered)
				if(m_bodyPartTypesWithRigidArmor.contains(bodyPartType))
					return false;
	}
	return true;
}
