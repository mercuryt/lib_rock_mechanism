#include "equipment.h"
#include "animalSpecies.h"
#include "area/area.h"
#include "config.h"
#include "items/items.h"
#include "random.h"
#include "hit.h"
#include "materialType.h"
#include "reference.h"
#include "types.h"
#include "weaponType.h"
#include "simulation/simulation.h"
#include "itemType.h"
#include "bodyType.h"
#include "actors/actors.h"
#include "hasShapes.hpp"
#include <cstddef>
EquipmentSet::EquipmentSet(Area& area, const Json& data)
{
	Items& items = area.getItems();
	for(const Json& itemRefData : data["m_equipments"])
		addEquipment(area, ItemReference(itemRefData, items.m_referenceData).getIndex(items.m_referenceData));
}
void EquipmentSet::addEquipment(Area& area, const ItemIndex& equipment)
{
	Items& items = area.getItems();
	ItemReference ref = area.getItems().getReference(equipment);
	assert(!contains(ref));
	assert(items.getLocation(equipment).empty());
	m_mass += items.getMass(equipment);
	m_equipments.insert(ref);
	ItemTypeId itemType = items.getItemType(equipment);
	auto& bodyParts = ItemType::getWearable_bodyPartsCovered(itemType);
	if(!bodyParts.empty())
	{
		m_wearableIsSorted = false;
		m_wearable.insert(ref);
		if(ItemType::getWearable_rigid(itemType))
			for(BodyPartTypeId bodyPartType : bodyParts)
			{
				assert(!m_bodyPartTypesWithRigidArmor.contains(bodyPartType));
				m_bodyPartTypesWithRigidArmor.insert(bodyPartType);
			}
	}
	if(ItemType::getIsWeapon(itemType))
	{
		if(ItemType::hasRangedAttack(itemType))
			m_rangedWeapons.insert(ref);
		if(ItemType::hasMeleeAttack(itemType))
			m_meleeWeapons.insert(ref);
	}
	//TODO: m_rangedWeaponAmmo
}
void EquipmentSet::addGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	Items& items = area.getItems();
	assert(!ItemType::getIsWearable(itemType));
	auto iter = m_equipments.findIf([&](const ItemReference& ref){
		ItemIndex item = ref.getIndex(items.m_referenceData);
		if(itemType != items.getItemType(item))
			return false;
		if(materialType != items.getMaterialType(item))
			return false;
		return true;
	});
	if(iter != m_equipments.end())
	{
		ItemReference existingRef = *iter;
		items.addQuantity(existingRef.getIndex(items.m_referenceData), quantity);
	}
	else
	{
		ItemIndex item = items.create({.itemType=itemType, .materialType=materialType, .quantity=quantity});
		m_equipments.insert(items.m_referenceData.getReference(item));
	}
	m_mass += ItemType::getFullDisplacement(itemType) * MaterialType::getDensity(materialType) * quantity;
}
void EquipmentSet::removeGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	Items& items = area.getItems();
	auto iter = m_equipments.findIf([&](const ItemReference& ref){
		ItemIndex item = ref.getIndex(items.m_referenceData);
		if(itemType != items.getItemType(item))
			return false;
		if(materialType != items.getMaterialType(item))
			return false;
		return true;
	});
	assert(iter != m_equipments.end());
	ItemReference ref = *iter;
	ItemIndex item = ref.getIndex(items.m_referenceData);
	assert(quantity <= items.getQuantity(item));
	if(quantity == items.getQuantity(item))
	{
		m_equipments.erase(iter);
		items.destroy(item);
		m_wearableIsSorted = false;
	}
	else
	{
		items.removeQuantity(item, quantity);
	}
	m_mass -= ItemType::getFullDisplacement(itemType) * MaterialType::getDensity(materialType) * quantity;
}
void EquipmentSet::removeEquipment(Area& area, const ItemIndex& equipment)
{
	Items& items = area.getItems();
	ItemReference equipmentRef = items.getReference(equipment);
	assert(contains(equipmentRef));
	assert(m_mass >= items.getMass(equipment));
	m_mass -= items.getMass(equipment);
	m_equipments.erase(equipmentRef);
	if(ItemType::getIsWearable(items.getItemType(equipment)))
		m_wearable.erase(equipmentRef);
	// SmallSet does not maintain sort order on erase.
	m_wearableIsSorted = false;
}
void EquipmentSet::modifyImpact(Area& area, Hit& hit, const BodyPartTypeId& bodyPartType)
{
	Items& items = area.getItems();
	if(!m_wearableIsSorted)
	{
		// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
		auto sort = [&area, &items](const ItemReference& a, const ItemReference& b) -> bool {
			return ItemType::getWearable_layer(items.getItemType(a.getIndex(items.m_referenceData))) > ItemType::getWearable_layer(items.getItemType(b.getIndex(items.m_referenceData)));
		};
		m_wearable.sort(sort);
		m_wearableIsSorted = true;
	}
	for(ItemReference equipment : m_wearable)
	{
		ItemIndex itemIndex = equipment.getIndex(items.m_referenceData);
		ItemTypeId itemType = area.getItems().getItemType(itemIndex);
		MaterialTypeId materialType = area.getItems().getMaterialType(itemIndex);
		Random& random = area.m_simulation.m_random;
		auto& bodyPartsCovered = ItemType::getWearable_bodyPartsCovered(itemType);
		auto chance = ItemType::getWearable_percentCoverage(itemType);
		if(std::ranges::find(bodyPartsCovered, bodyPartType) != bodyPartsCovered.end() && random.percentChance(chance))
		{
			auto hardness = MaterialType::getHardness(materialType);
			uint32_t pierceScore = ((float)hit.force.get() / hit.area) * MaterialType::getHardness(hit.materialType) * Config::pierceModifier;
			uint32_t defenseScore = ItemType::getWearable_defenseScore(itemType) * hardness;
			if(pierceScore < defenseScore)
			{
				if(ItemType::getWearable_rigid(itemType))
					hit.area = Config::convertBodyPartVolumeToArea(BodyPartType::getVolume(bodyPartType));
				auto modifier = ItemType::getWearable_forceAbsorbedUnpiercedModifier(itemType);
				hit.force -= modifier * hardness * Config::forceAbsorbedUnpiercedModifier;
			}
			else
			{
				//TODO: Add wear to equipment.
				auto modifier = ItemType::getWearable_forceAbsorbedPiercedModifier(itemType);
				hit.force -= modifier * hardness * Config::forceAbsorbedPiercedModifier;
			}

		}
	}
	m_equipments.eraseIf([&](const ItemReference& equipment){ return area.getItems().getWear(equipment.getIndex(items.m_referenceData)) == 100; });
}
std::vector<Attack> EquipmentSet::getMeleeAttacks(Area& area)
{
	std::vector<Attack> output;
	Items& items = area.getItems();
	for(ItemReference equipment : m_meleeWeapons)
	{
		ItemIndex itemIndex = equipment.getIndex(items.m_referenceData);
		ItemTypeId itemType = area.getItems().getItemType(itemIndex);
		if(ItemType::getIsWeapon(itemType))
			for(AttackTypeId attackType : ItemType::getAttackTypes(itemType))
				output.emplace_back(attackType, area.getItems().getMaterialType(itemIndex), itemIndex);
	}
	return output;
}
bool EquipmentSet::contains(const ItemReference& item) const
{
	return m_equipments.contains(item);
}
bool EquipmentSet::containsItemType(const Area& area, const ItemTypeId& itemType) const
{
	const Items& items = area.getItems();
	return m_equipments.anyOf([&items, itemType](const ItemReference& item){ return items.getItemType(item.getIndex(items.m_referenceData)) == itemType; });
}
Step EquipmentSet::getLongestMeleeWeaponCoolDown(Area& area) const
{
	assert(!m_meleeWeapons.empty());
	Step output = Step::create(0);
	const Items& items = area.getItems();
	for(ItemReference equipment : m_meleeWeapons)
	{
		ItemTypeId itemType = area.getItems().getItemType(equipment.getIndex(items.m_referenceData));
		if(ItemType::getIsWeapon(itemType))
			output = std::max(output, ItemType::getAttackCoolDownBase(itemType));
	}
	assert(output != 0);
	return output;
}
bool EquipmentSet::canEquipCurrently(Area& area, const ActorIndex& actor, const ItemIndex& equipment) const
{
	Items& items = area.getItems();
	const ItemReference equipmentRef = items.m_referenceData.getReference(equipment);
	assert(!m_equipments.contains(equipmentRef));
	const ItemTypeId itemType = area.getItems().getItemType(equipment);
	const AnimalSpeciesId species = area.getActors().getSpecies(actor);
	if(ItemType::getIsWearable(itemType))
	{
		const auto& bodyPartsCovered = ItemType::getWearable_bodyPartsCovered(itemType);
		for(BodyPartTypeId bodyPartType : bodyPartsCovered)
			if(!BodyType::hasBodyPart(AnimalSpecies::getBodyType(species), bodyPartType))
				return false;
		if(ItemType::getWearable_bodyTypeScale(itemType) != AnimalSpecies::getBodyScale(species))
			return false;
		if(ItemType::getWearable_rigid(itemType))
			for(BodyPartTypeId bodyPartType : bodyPartsCovered)
				if(m_bodyPartTypesWithRigidArmor.contains(bodyPartType))
					return false;
	}
	return true;
}
ItemIndex EquipmentSet::getWeaponToAttackAtRange(Area& area, const DistanceInBlocksFractional& range)
{
	Items& items = area.getItems();
	for(ItemReference item : m_rangedWeapons)
	{
		ItemIndex itemIndex = item.getIndex(items.m_referenceData);
		ItemTypeId itemType = items.getItemType(itemIndex);
		assert(ItemType::getIsWearable(itemType));
		for(AttackTypeId attackType : ItemType::getAttackTypes(itemType))
			if(AttackType::getRange(attackType) >= range)
				return itemIndex;
	}
	return ItemIndex::null();
}
ItemIndex EquipmentSet::getAmmoForRangedWeapon(Area& area, const ItemIndex& weapon)
{
	Items& items = area.getItems();
	ItemTypeId weaponType = items.getItemType(weapon);
	assert(ItemType::getIsWeapon(weaponType));
	AttackTypeId attackType = ItemType::getRangedAttackType(weaponType);
	assert(AttackType::getProjectileItemType(attackType).exists());
	ItemTypeId ammoItemType = AttackType::getProjectileItemType(attackType);
	for(ItemReference item : m_equipments)
	{
		ItemIndex itemIndex = item.getIndex(items.m_referenceData);
		if(items.getItemType(itemIndex) == ammoItemType)
			return itemIndex;
	}
	return ItemIndex::null();
}
bool EquipmentSet::hasAnyEquipmentWithReservations(Area& area, const ActorIndex& actor) const
{
	FactionId faction = area.getActors().getFaction(actor);
	Items& items = area.getItems();
	if(faction.exists())
		for(ItemReference item : m_equipments)
			if(area.getItems().reservable_isFullyReserved(item.getIndex(items.m_referenceData), faction))
				return true;
	return false;
}
void EquipmentSet::updateCarrierIndexForContents(Area& area, const ItemIndex& newIndex)
{
	Items& items = area.getItems();
	for(ItemReference item : m_equipments)
		items.updateCarrierIndex(item.getIndex(items.m_referenceData), newIndex);
}
ItemIndex EquipmentSet::getFirstItemWithType(const Area& area, const ItemTypeId& type) const
{
	const Items& items = area.getItems();
	auto found = m_equipments.findIf([&](const ItemReference& ref) -> bool {
		const ItemIndex& index = ref.getIndex(items.m_referenceData);
		return items.getItemType(index) == type;
	});
	if(found == m_equipments.end())
		return ItemIndex::null();
	else
		return found->getIndex(items.m_referenceData);
}