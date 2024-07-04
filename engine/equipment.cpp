#include "equipment.h"
#include "animalSpecies.h"
#include "area.h"
#include "config.h"
#include "items/items.h"
#include "random.h"
#include "hit.h"
#include "materialType.h"
#include "types.h"
#include "weaponType.h"
#include "simulation.h"
#include "itemType.h"
#include "actors/actors.h"
#include <cstddef>
/*
EquipmentSet::EquipmentSet(const Json& data, Actor& a) : m_actor(a)
{
	for(const Json& equipmentId : data["equipments"])
	{
		Item& equipment = m_actor.getSimulation().m_hasItems->getById(equipmentId.get<ItemId>());
		addEquipment(equipment);
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
*/
bool EquipmentSortByLayer::operator()(ItemIndex const& a, ItemIndex const& b) const
{
	const ItemType& aType = items.getItemType(a);
	const ItemType& bType = items.getItemType(b);
	assert(aType.wearableData != nullptr);
	assert(bType.wearableData != nullptr);
	return aType.wearableData->layer > bType.wearableData->layer;
}
void EquipmentSet::addEquipment(Area& area, ItemIndex equipment)
{
	assert(!m_equipments.contains(equipment));
	Items& items = area.getItems();
	assert(items.getLocation(equipment) == BLOCK_INDEX_MAX);
	m_mass += items.getMass(equipment);
	m_equipments.insert(equipment);
	const ItemType& itemType = items.getItemType(equipment);
	if(itemType.wearableData != nullptr)
	{
		m_wearable.insert(equipment);
		if(itemType.wearableData->rigid)
			for(const BodyPartType* bodyPartType : itemType.wearableData->bodyPartsCovered)
			{
				assert(!m_bodyPartTypesWithRigidArmor.contains(bodyPartType));
				m_bodyPartTypesWithRigidArmor.insert(bodyPartType);
			}
	}
	if(itemType.weaponData != nullptr)
	{
		if(itemType.hasRangedAttack())
			m_rangedWeapons.insert(equipment);
		if(itemType.hasMeleeAttack())
			m_meleeWeapons.insert(equipment);
	}
	//TODO: m_rangedWeaponAmmo
}
void EquipmentSet::addGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	auto found = std::ranges::find_if(m_equipments, [&](const ItemIndex& item) {
		Items& items = area.getItems();
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	if(found == m_equipments.end())
	{
		ItemIndex equipment = area.getItems().create({
			.itemType = itemType,
			.materialType = materialType,
			.quantity = quantity,
		});
		addEquipment(area, equipment);
	}
	else
	{
		ItemIndex equipment = *found;
		area.getItems().addQuantity(equipment, quantity);
	}
}
void EquipmentSet::removeGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	Items& items = area.getItems();
	auto found = std::ranges::find_if(m_equipments, [&](const ItemIndex& item) {
		return items.getItemType(item) == itemType && items.getMaterialType(item) == materialType;
	});
	assert(found != m_equipments.end());
	items.removeQuantity(*found, quantity);
}
void EquipmentSet::removeEquipment(Area& area, ItemIndex equipment)
{
	assert(m_equipments.contains(equipment));
	assert(m_mass >= area.getItems().getMass(equipment));
	m_mass -= area.getItems().getMass(equipment);
	m_equipments.erase(equipment);
	if(area.getItems().getItemType(equipment).wearableData != nullptr)
		m_wearable.erase(equipment);
}
void EquipmentSet::modifyImpact(Area& area, Hit& hit, const BodyPartType& bodyPartType)
{
	// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
	for(ItemIndex equipment : m_wearable)
	{
		const ItemType& itemType = area.getItems().getItemType(equipment);
		const MaterialType& materialType = area.getItems().getMaterialType(equipment);
		auto& wearableData = *itemType.wearableData;
		Random& random = area.m_simulation.m_random;
		if(std::ranges::find(wearableData.bodyPartsCovered, &bodyPartType) != wearableData.bodyPartsCovered.end() && random.percentChance(wearableData.percentCoverage))
		{
			uint32_t pierceScore = ((float)hit.force / hit.area) * hit.materialType.hardness * Config::pierceModifier;
			uint32_t defenseScore = wearableData.defenseScore * materialType.hardness;
			if(pierceScore < defenseScore)
			{
				if(wearableData.rigid)
					hit.area = Config::convertBodyPartVolumeToArea(bodyPartType.volume);
				hit.force -= wearableData.forceAbsorbedUnpiercedModifier * materialType.hardness * Config::forceAbsorbedUnpiercedModifier;
			}
			else
			{
				//TODO: Add wear to equipment.
				hit.force -= wearableData.forceAbsorbedPiercedModifier * materialType.hardness * Config::forceAbsorbedPiercedModifier;
			}

		}
	}
	std::erase_if(m_equipments, [&](ItemIndex equipment){ return area.getItems().getWear(equipment) == 100; });
}
std::vector<Attack> EquipmentSet::getMeleeAttacks(Area& area)
{
	std::vector<Attack> output;
	for(ItemIndex equipment : m_meleeWeapons)
	{
		const ItemType& itemType = area.getItems().getItemType(equipment);
		if(itemType.weaponData != nullptr)
			for(const AttackType& attackType : itemType.weaponData->attackTypes)
				output.emplace_back(&attackType, &area.getItems().getMaterialType(equipment), equipment);
	}
	return output;
}
bool EquipmentSet::containsItemType(const Area& area, const ItemType& itemType) const
{
	const Items& items = area.getItems();
	return std::ranges::find_if(m_equipments, [&items, itemType](const ItemIndex& item){ return items.getItemType(item) == itemType; }) != m_equipments.end();
}
Step EquipmentSet::getLongestMeleeWeaponCoolDown(Area& area) const
{
	assert(!m_meleeWeapons.empty());
	Step output = 0;
	for(ItemIndex equipment : m_meleeWeapons)
	{
		const ItemType& itemType = area.getItems().getItemType(equipment);
		if(itemType.weaponData != nullptr)
			output = std::max(output, itemType.weaponData->coolDown);
	}
	assert(output != 0);
	return output;
}
bool EquipmentSet::canEquipCurrently(const Area& area, ActorIndex actor, ItemIndex equipment) const
{
	assert(!m_equipments.contains(equipment));
	const ItemType& itemType = area.getItems().getItemType(equipment);
	const AnimalSpecies& species = area.getActors().getSpecies(actor);
	if(itemType.wearableData != nullptr)
	{
		for(const BodyPartType* bodyPartType : itemType.wearableData->bodyPartsCovered)
			if(!species.bodyType.hasBodyPart(*bodyPartType))
				return false;
		if(&itemType.wearableData->bodyTypeScale != &species.bodyScale)
			return false;
		if(itemType.wearableData->rigid)
			for(const BodyPartType* bodyPartType : itemType.wearableData->bodyPartsCovered)
				if(m_bodyPartTypesWithRigidArmor.contains(bodyPartType))
					return false;
	}
	return true;
}
ItemIndex EquipmentSet::getWeaponToAttackAtRange(Area& area, float range)
{
	for(ItemIndex item : m_rangedWeapons)
	{
		const ItemType& itemType = area.getItems().getItemType(item);
		assert(itemType.weaponData != nullptr);
		for(const AttackType& attackType : itemType.weaponData->attackTypes)
			if(attackType.range >= range)
				return item;
	}
	return ITEM_INDEX_MAX;
}
ItemIndex EquipmentSet::getAmmoForRangedWeapon(Area& area, ItemIndex weapon)
{
	const ItemType& weaponType = area.getItems().getItemType(weapon);
	assert(weaponType.weaponData != nullptr);
	const AttackType* attackType = weaponType.getRangedAttackType();
	assert(attackType->projectileItemType != nullptr);
	const ItemType& ammoItemType = *attackType->projectileItemType;
	for(ItemIndex item : m_equipments)
		if(area.getItems().getItemType(item) == ammoItemType)
			return item;
	return ITEM_INDEX_MAX;
}
bool EquipmentSet::hasAnyEquipmentWithReservations(Area& area, ActorIndex actor) const
{
	const Faction* faction = area.getActors().getFaction(actor);
	if(faction != nullptr)
		for(const ItemIndex item : m_equipments)
			if(area.getItems().reservable_isFullyReserved(item, *faction))
				return true;
	return false;
}
