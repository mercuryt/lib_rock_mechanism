#include "actors.h"
#include "../types.h"
#include "../area/area.h"
#include "../items/items.h"
void Actors::equipment_add(const ActorIndex& index, const ItemIndex& item)
{
	m_equipmentSet[index]->addEquipment(m_area, item);
	combat_update(index);
}
void Actors::equipment_addGeneric(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materalType, const Quantity& quantity)
{
	m_equipmentSet[index]->addGeneric(m_area, itemType, materalType, quantity);
}
void Actors::equipment_remove(const ActorIndex& index, const ItemIndex& item)
{
	m_equipmentSet[index]->removeEquipment(m_area, item);
}
void Actors::equipment_removeGeneric(const ActorIndex& index, const ItemTypeId& itemType, const MaterialTypeId& materalType, const Quantity& quantity)
{
	m_equipmentSet[index]->removeGeneric(m_area, itemType, materalType, quantity);
}
bool Actors::equipment_canEquipCurrently(const ActorIndex& index, const ItemIndex& item) const
{
	return m_equipmentSet[index]->canEquipCurrently(m_area, index, item);
}
bool Actors::equipment_containsItem(const ActorIndex& index, const ItemIndex& item) const
{
	return m_equipmentSet[index]->contains(m_area.getItems().m_referenceData.getReference(item));
}
bool Actors::equipment_containsItemType(const ActorIndex& index, const ItemTypeId& type) const
{
	return m_equipmentSet[index]->containsItemType(m_area, type);
}
Mass Actors::equipment_getMass(const ActorIndex& index) const
{
	return m_equipmentSet[index]->getMass();
}
ItemIndex Actors::equipment_getWeaponToAttackAtRange(const ActorIndex& index, const DistanceInBlocksFractional& range) const
{
	return m_equipmentSet[index]->getWeaponToAttackAtRange(m_area, range);
}
ItemIndex Actors::equipment_getAmmoForRangedWeapon(const ActorIndex& index, const ItemIndex& weapon) const
{
	return m_equipmentSet[index]->getAmmoForRangedWeapon(m_area, weapon);
}
