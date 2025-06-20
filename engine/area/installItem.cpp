#include "installItem.h"
#include "actors/actors.h"
#include "area/area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "reference.h"
#include "path/terrainFacade.h"
#include "definitions/moveType.h"

// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, const BlockIndex& block, const ItemIndex& item, const Facing4& facing, const FactionId& faction)
{
	assert(!m_designations.contains(block));
	Items& items = area.getItems();
	const SmallSet<BlockIndex>& occupied = Shape::getBlocksOccupiedAt(items.getCompoundShape(item), area.getBlocks(), block, facing);
	m_designations.emplace(block, area, items.getReference(item), block, facing, faction, occupied);
}
void AreaHasInstallItemDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_designations)
			pair2.second->clearReservations();
}
void HasInstallItemDesignationsForFaction::remove(Area& area, const ItemIndex& item)
{
	assert(m_designations.contains(area.getItems().getLocation(item)));
	m_designations.erase(area.getItems().getLocation(item));
}
