#include "actors.h"
#include "types.h"
void Actors::equipment_add(ActorIndex index, ItemIndex item)
{
	m_equipmentSet[index]->addEquipment(m_area, item);
}
void Actors::equipment_addGeneric(ActorIndex index, ItemTypeId itemType, MaterialTypeId materalType, Quantity quantity)
{
	m_equipmentSet[index]->addGeneric(m_area, itemType, materalType, quantity);
}
void Actors::equipment_remove(ActorIndex index, ItemIndex item)
{
	m_equipmentSet[index]->removeEquipment(m_area, item);
}
void Actors::equipment_removeGeneric(ActorIndex index, ItemTypeId itemType, MaterialTypeId materalType, Quantity quantity)
{
	m_equipmentSet[index]->removeGeneric(m_area, itemType, materalType, quantity);
}
bool Actors::equipment_canEquipCurrently(ActorIndex index, ItemIndex item) const
{
	return m_equipmentSet[index]->canEquipCurrently(m_area, index, item);
}
bool Actors::equipment_containsItem(ActorIndex index, ItemIndex item) const
{
	return m_equipmentSet[index]->contains(item);
}
bool Actors::equipment_containsItemType(ActorIndex index, ItemTypeId type) const
{
	return m_equipmentSet[index]->containsItemType(m_area, type);
}
Mass Actors::equipment_getMass(ActorIndex index) const
{
	return m_equipmentSet[index]->getMass();
}
ItemIndex Actors::equipment_getWeaponToAttackAtRange(ActorIndex index, DistanceInBlocksFractional range) const
{
	return m_equipmentSet[index]->getWeaponToAttackAtRange(m_area, range);
}
ItemIndex Actors::equipment_getAmmoForRangedWeapon(ActorIndex index, ItemIndex weapon) const
{
	return m_equipmentSet[index]->getAmmoForRangedWeapon(m_area, weapon);
} 
