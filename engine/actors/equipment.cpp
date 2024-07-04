#include "actors.h"
void Actors::equipment_add(ActorIndex index, ItemIndex item)
{
	m_equipmentSet.at(index).addEquipment(m_area, item);
}
void Actors::equipment_addGeneric(ActorIndex index, const ItemType& itemType, const MaterialType& materalType, Quantity quantity)
{
	m_equipmentSet.at(index).addGeneric(m_area, itemType, materalType, quantity);
}
void Actors::equipment_remove(ActorIndex index, ItemIndex item)
{
	m_equipmentSet.at(index).removeEquipment(m_area, item);
}
void Actors::equipment_removeGeneric(ActorIndex index, const ItemType& itemType, const MaterialType& materalType, Quantity quantity)
{
	m_equipmentSet.at(index).removeGeneric(m_area, itemType, materalType, quantity);
}
bool Actors::equipment_canEquipCurrently(ActorIndex index, ItemIndex item) const
{
	return m_equipmentSet.at(index).canEquipCurrently(m_area, index, item);
}
bool Actors::equipment_containsItem(ActorIndex index, ItemIndex item) const
{
	return m_equipmentSet.at(index).contains(item);
}
bool Actors::equipment_containsItemType(ActorIndex index, const ItemType& type) const
{
	return m_equipmentSet.at(index).containsItemType(type);
}
Mass Actors::equipment_getMass(ActorIndex index) const
{
	return m_equipmentSet.at(index).getMass();
}
std::unordered_set<ItemIndex>& Actors::equipment_getAll(ActorIndex index)
{
	return 	m_equipmentSet.at(index).getAll();
}
const std::unordered_set<ItemIndex>& Actors::equipment_getAll(ActorIndex index) const
{
	return 	const_cast<EquipmentSet&>(m_equipmentSet.at(index)).getAll();
}
ItemIndex Actors::equipment_getWeaponToAttackAtRange(ActorIndex index, DistanceInBlocks range)
{
	return m_equipmentSet.at(index).getWeaponToAttackAtRange(m_area, range);
}
ItemIndex Actors::equipment_getAmmoForRangedWeapon(ActorIndex index, ItemIndex weapon)
{
	return m_equipmentSet.at(index).getAmmoForRangedWeapon(m_area, weapon);
}
