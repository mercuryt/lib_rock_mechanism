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
#include "bodyType.h"
#include "actors/actors.h"
#include <cstddef>
EquipmentSet::EquipmentSet(Area& area, const Json& data)
{
	for(const Json& itemIndex : data["m_equipments"]["data"])
		addEquipment(area, itemIndex.get<ItemIndex>());
}
void EquipmentSet::addEquipment(Area& area, const ItemIndex& equipment)
{
	assert(!contains(equipment));
	Items& items = area.getItems();
	assert(items.getLocation(equipment).empty());
	m_mass += items.getMass(equipment);
	ItemReference ref = area.getItems().getReference(equipment);
	m_equipments.add(ref);
	ItemTypeId itemType = items.getItemType(equipment);
	auto& bodyParts = ItemType::getWearable_bodyPartsCovered(itemType);
	if(!bodyParts.empty())
	{
		m_wearable.add(ref);
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
			m_rangedWeapons.add(ref);
		if(ItemType::hasMeleeAttack(itemType))
			m_meleeWeapons.add(ref);
	}
	//TODO: m_rangedWeaponAmmo
}
void EquipmentSet::addGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	m_equipments.addGeneric(area, itemType, materialType, quantity);
	m_mass += ItemType::getVolume(itemType) * MaterialType::getDensity(materialType) * quantity;
}
void EquipmentSet::removeGeneric(Area& area, const ItemTypeId& itemType, const MaterialTypeId& materialType, const Quantity& quantity)
{
	m_equipments.addGeneric(area, itemType, materialType, quantity);
	m_mass -= ItemType::getVolume(itemType) * MaterialType::getDensity(materialType) * quantity;
}
void EquipmentSet::removeEquipment(Area& area, const ItemIndex& equipment)
{
	assert(contains(equipment));
	assert(m_mass >= area.getItems().getMass(equipment));
	m_mass -= area.getItems().getMass(equipment);
	m_equipments.remove(equipment);
	if(ItemType::getIsWearable(area.getItems().getItemType(equipment)))
		m_wearable.remove(equipment);
}
void EquipmentSet::modifyImpact(Area& area, Hit& hit, const BodyPartTypeId& bodyPartType)
{
	if(!m_wearable.isSorted())
	{
		// Wearable priority queue is sorted by layers so we start at the outside and pierce inwards.
		auto sort = [&area](const ItemReference& a, const ItemReference& b){
			Items& items = area.getItems();
			return ItemType::getWearable_layer(items.getItemType(a.getIndex())) > ItemType::getWearable_layer(items.getItemType(b.getIndex()));
		};
		m_wearable.sort(sort);
	}
	for(ItemReference equipment : m_wearable)
	{
		ItemIndex itemIndex = equipment.getIndex();
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
	m_equipments.removeIf([&](const ItemReference& equipment){ return area.getItems().getWear(equipment.getIndex()) == 100; });
}
std::vector<Attack> EquipmentSet::getMeleeAttacks(Area& area)
{
	std::vector<Attack> output;
	for(ItemReference equipment : m_meleeWeapons)
	{
		ItemIndex itemIndex = equipment.getIndex();
		ItemTypeId itemType = area.getItems().getItemType(itemIndex);
		if(ItemType::getIsWeapon(itemType))
			for(AttackTypeId attackType : ItemType::getAttackTypes(itemType))
				output.emplace_back(attackType, area.getItems().getMaterialType(itemIndex), itemIndex);
	}
	return output;
}
bool EquipmentSet::contains(const ItemIndex& item) const
{
	return m_equipments.contains(item);
}
bool EquipmentSet::containsItemType(const Area& area, const ItemTypeId& itemType) const
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
		ItemTypeId itemType = area.getItems().getItemType(equipment.getIndex());
		if(ItemType::getIsWeapon(itemType))
			output = std::max(output, ItemType::getAttackCoolDownBase(itemType));
	}
	assert(output != 0);
	return output;
}
bool EquipmentSet::canEquipCurrently(const Area& area, const ActorIndex& actor, const ItemIndex& equipment) const
{
	assert(!m_equipments.contains(equipment));
	ItemTypeId itemType = area.getItems().getItemType(equipment);
	AnimalSpeciesId species = area.getActors().getSpecies(actor);
	if(ItemType::getIsWearable(itemType))
	{
		auto& bodyPartsCovered = ItemType::getWearable_bodyPartsCovered(itemType);
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
	for(ItemReference item : m_rangedWeapons)
	{
		ItemIndex itemIndex = item.getIndex();
		ItemTypeId itemType = area.getItems().getItemType(itemIndex);
		assert(ItemType::getIsWearable(itemType));
		for(AttackTypeId attackType : ItemType::getAttackTypes(itemType))
			if(AttackType::getRange(attackType) >= range)
				return itemIndex;
	}
	return ItemIndex::null();
}
ItemIndex EquipmentSet::getAmmoForRangedWeapon(Area& area, const ItemIndex& weapon)
{
	ItemTypeId weaponType = area.getItems().getItemType(weapon);
	assert(ItemType::getIsWeapon(weaponType));
	AttackTypeId attackType = ItemType::getRangedAttackType(weaponType);
	assert(AttackType::getProjectileItemType(attackType).exists());
	ItemTypeId ammoItemType = AttackType::getProjectileItemType(attackType);
	for(ItemReference item : m_equipments)
	{
		ItemIndex itemIndex = item.getIndex();
		if(area.getItems().getItemType(itemIndex) == ammoItemType)
			return itemIndex;
	}
	return ItemIndex::null();
}
bool EquipmentSet::hasAnyEquipmentWithReservations(Area& area, const ActorIndex& actor) const
{
	FactionId faction = area.getActors().getFaction(actor);
	if(faction.exists())
		for(const ItemReference item : m_equipments)
			if(area.getItems().reservable_isFullyReserved(item.getIndex(), faction))
				return true;
	return false;
}
void EquipmentSet::updateCarrierIndexForContents(Area& area, const ItemIndex& newIndex)
{
	Items& items = area.getItems();
	for(ItemReference item : m_equipments)
		items.updateCarrierIndex(item.getIndex(), newIndex);
}
