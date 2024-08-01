#include "equipment.h"
#include "animalSpecies.h"
#include "area.h"
#include "config.h"
#include "items/items.h"
#include "random.h"
#include "hit.h"
#include "materialType.h"
#include "reference.h"
#include "types.h"
#include "weaponType.h"
#include "simulation.h"
#include "itemType.h"
#include "actors/actors.h"
#include <cstddef>
EquipmentSet::EquipmentSet(Area& area, const Json& data)
{
	for(const Json& itemIndex : data["equipments"])
		addEquipment(area, itemIndex.get<ItemIndex>());
}
void EquipmentSet::addEquipment(Area& area, ItemIndex equipment)
{
	assert(!contains(equipment));
	Items& items = area.getItems();
	assert(items.getLocation(equipment).empty());
	m_mass += items.getMass(equipment);
	ItemReference ref = area.getItems().getReference(equipment);
	m_equipments.add(ref);
	const ItemType& itemType = items.getItemType(equipment);
	if(itemType.wearableData != nullptr)
	{
		m_wearable.add(ref);
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
			m_rangedWeapons.add(ref);
		if(itemType.hasMeleeAttack())
			m_meleeWeapons.add(ref);
	}
	//TODO: m_rangedWeaponAmmo
}
void EquipmentSet::addGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	m_equipments.addGeneric(area, itemType, materialType, quantity);
	m_mass += itemType.volume * materialType.density * quantity;
}
void EquipmentSet::removeGeneric(Area& area, const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	m_equipments.addGeneric(area, itemType, materialType, quantity);
	m_mass -= itemType.volume * materialType.density * quantity;
}
void EquipmentSet::removeEquipment(Area& area, ItemIndex equipment)
{
	assert(contains(equipment));
	assert(m_mass >= area.getItems().getMass(equipment));
	m_mass -= area.getItems().getMass(equipment);
	m_equipments.remove(equipment);
	if(area.getItems().getItemType(equipment).wearableData != nullptr)
		m_wearable.remove(equipment);
}
void EquipmentSet::modifyImpact(Area& area, Hit& hit, const BodyPartType& bodyPartType)
{
	if(!m_wearable.isSorted())
	{
		// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
		auto sort = [&area](const ItemReference& a, const ItemReference& b){
			Items& items = area.getItems();
			return items.getItemType(a.getIndex()).wearableData->layer > items.getItemType(b.getIndex()).wearableData->layer;
		};
		m_wearable.sort(sort);
	}
	for(ItemReference equipment : m_wearable)
	{
		ItemIndex itemIndex = equipment.getIndex();
		const ItemType& itemType = area.getItems().getItemType(itemIndex);
		const MaterialType& materialType = area.getItems().getMaterialType(itemIndex);
		auto& wearableData = *itemType.wearableData;
		Random& random = area.m_simulation.m_random;
		if(std::ranges::find(wearableData.bodyPartsCovered, &bodyPartType) != wearableData.bodyPartsCovered.end() && random.percentChance(wearableData.percentCoverage))
		{
			uint32_t pierceScore = ((float)hit.force.get() / hit.area) * hit.materialType.hardness * Config::pierceModifier;
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
	m_equipments.removeIf([&](const ItemReference& equipment){ return area.getItems().getWear(equipment.getIndex()) == 100; });
}
std::vector<Attack> EquipmentSet::getMeleeAttacks(Area& area)
{
	std::vector<Attack> output;
	for(ItemReference equipment : m_meleeWeapons)
	{
		ItemIndex itemIndex = equipment.getIndex();
		const ItemType& itemType = area.getItems().getItemType(itemIndex);
		if(itemType.weaponData != nullptr)
			for(const AttackType& attackType : itemType.weaponData->attackTypes)
				output.emplace_back(&attackType, &area.getItems().getMaterialType(itemIndex), itemIndex);
	}
	return output;
}
bool EquipmentSet::contains(ItemIndex item) const
{
	return m_equipments.contains(item);
}
bool EquipmentSet::containsItemType(const Area& area, const ItemType& itemType) const
{
	const Items& items = area.getItems();
	return m_equipments.contains([&items, itemType](const ItemReference& item){ return items.getItemType(item.getIndex()) == itemType; });
}
Step EquipmentSet::getLongestMeleeWeaponCoolDown(Area& area) const
{
	assert(!m_meleeWeapons.empty());
	Step output = Step::create(0);
	for(ItemReference equipment : m_meleeWeapons)
	{
		const ItemType& itemType = area.getItems().getItemType(equipment.getIndex());
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
ItemIndex EquipmentSet::getWeaponToAttackAtRange(Area& area, DistanceInBlocksFractional range)
{
	for(ItemReference item : m_rangedWeapons)
	{
		ItemIndex itemIndex = item.getIndex();
		const ItemType& itemType = area.getItems().getItemType(itemIndex);
		assert(itemType.weaponData != nullptr);
		for(const AttackType& attackType : itemType.weaponData->attackTypes)
			if(attackType.range >= range)
				return itemIndex;
	}
	return ItemIndex::null();
}
ItemIndex EquipmentSet::getAmmoForRangedWeapon(Area& area, ItemIndex weapon)
{
	const ItemType& weaponType = area.getItems().getItemType(weapon);
	assert(weaponType.weaponData != nullptr);
	const AttackType* attackType = weaponType.getRangedAttackType();
	assert(attackType->projectileItemType != nullptr);
	const ItemType& ammoItemType = *attackType->projectileItemType;
	for(ItemReference item : m_equipments)
	{
		ItemIndex itemIndex = item.getIndex();
		if(area.getItems().getItemType(itemIndex) == ammoItemType)
			return itemIndex;
	}
	return ItemIndex::null();
}
bool EquipmentSet::hasAnyEquipmentWithReservations(Area& area, ActorIndex actor) const
{
	const Faction* faction = area.getActors().getFaction(actor);
	if(faction != nullptr)
		for(const ItemReference item : m_equipments)
			if(area.getItems().reservable_isFullyReserved(item.getIndex(), faction->id))
				return true;
	return false;
}
void EquipmentSet::updateCarrierIndexForContents(Area& area, ItemIndex newIndex)
{
	Items& items = area.getItems();
	for(ItemReference item : m_equipments)
		items.updateCarrierIndex(item.getIndex(), newIndex);
}
