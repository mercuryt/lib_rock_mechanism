#include "equipment.h"
#include "config.h"
#include "random.h"
#include "hit.h"
#include "materialType.h"
#include "simulation/hasItems.h"
#include "weaponType.h"
#include "actor.h"
#include "simulation.h"
#include <cstddef>

EquipmentSet::EquipmentSet(const Json& data, Actor& a) : m_actor(a), m_mass(0)
{
	for(const Json& equipmentId : data["equipments"])
	{
		Item& equipment = m_actor.getSimulation().m_hasItems->getById(equipmentId.get<ItemId>());
		insertEquipment(equipment);
	}
}
Json EquipmentSet::toJson() const 
{
	Json data;
	data["equipments"] = Json::array();
	for(const Item* equipment : m_equipments)
		data["equipments"].push_back(equipment->m_id);
	return data;
}
bool EquipmentSortByLayer::operator()(Item* const& a, Item* const& b) const
{
	assert(a->m_itemType.wearableData != nullptr);
	assert(b->m_itemType.wearableData != nullptr);
	return a->m_itemType.wearableData->layer > b->m_itemType.wearableData->layer;
}

void EquipmentSet::insertEquipment(Item& equipment)
{
	assert(std::ranges::find(m_equipments, &equipment) == m_equipments.end());
	assert(equipment.m_location == nullptr);
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
	if(equipment.m_itemType.weaponData != nullptr)
	{
		if(equipment.m_itemType.hasRangedAttack())
			m_rangedWeapons.insert(&equipment);
		if(equipment.m_itemType.hasMeleeAttack())
			m_meleeWeapons.insert(&equipment);
	}
}
void EquipmentSet::addEquipment(Item& equipment)
{
	insertEquipment(equipment);
	m_actor.m_canFight.update();
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
void EquipmentSet::modifyImpact(Hit& hit, const BodyPartType& bodyPartType)
{
	// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
	for(Item* equipment : m_wearable)
	{
		auto& wearableData = *equipment->m_itemType.wearableData;
		Random& random = m_actor.getSimulation().m_random;
		if(std::ranges::find(wearableData.bodyPartsCovered, &bodyPartType) != wearableData.bodyPartsCovered.end() && random.percentChance(wearableData.percentCoverage))
		{
			uint32_t pierceScore = ((float)hit.force / hit.area) * hit.materialType.hardness * Config::pierceModifier;
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
std::vector<Attack> EquipmentSet::getMeleeAttacks()
{
	std::vector<Attack> output;
	for(Item* equipment : m_meleeWeapons)
		if(equipment->m_itemType.weaponData != nullptr)
			for(const AttackType& attackType : equipment->m_itemType.weaponData->attackTypes)
				output.emplace_back(&attackType, &equipment->m_materialType, equipment);
	return output;
}
bool EquipmentSet::hasWeapons() const
{
	for(Item* equipment : m_equipments)
		if(equipment->m_itemType.weaponData != nullptr)
			return true;
	return false;
}
Step EquipmentSet::getLongestMeleeWeaponCoolDown() const
{
	assert(!m_meleeWeapons.empty());
	Step output = 0;
	for(Item* equipment : m_meleeWeapons)
		if(equipment->m_itemType.weaponData != nullptr)
			output = std::max(output, equipment->m_itemType.weaponData->coolDown);
	assert(output != 0);
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
Item* EquipmentSet::getWeaponToAttackAtRange(float range)
{
	for(Item* item : m_rangedWeapons)
	{
		assert(item->m_itemType.weaponData != nullptr);
		for(const AttackType& attackType : item->m_itemType.weaponData->attackTypes)
			if(attackType.range >= range)
				return item;
	}
	return nullptr;
}
Item* EquipmentSet::getAmmoForRangedWeapon(Item& weapon)
{
	assert(weapon.m_itemType.weaponData != nullptr);
	const AttackType* attackType = weapon.m_itemType.getRangedAttackType();
	assert(attackType->projectileItemType != nullptr);
	const ItemType& ammoItemType = *attackType->projectileItemType;
	for(Item* item : m_equipments)
		if(item->m_itemType == ammoItemType)
			return item;
	return nullptr;
}
bool EquipmentSet::hasAnyEquipmentWithReservations() const
{
	if(m_actor.getFaction())
		for(const Item* item : m_equipments)
			if(item->m_reservable.hasAnyReservationsWith(*m_actor.getFaction()))
				return true;
	return false;
}
