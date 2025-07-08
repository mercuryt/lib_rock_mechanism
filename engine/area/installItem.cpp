#include "installItem.h"
#include "actors/actors.h"
#include "area/area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "reference.h"
#include "path/terrainFacade.h"
#include "definitions/moveType.h"

// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, const Point3D& point, const ItemIndex& item, const Facing4& facing, const FactionId& faction)
{
	assert(!m_designations.contains(point));
	Items& items = area.getItems();
	const SmallSet<Point3D>& occupied = Shape::getPointsOccupiedAt(items.getCompoundShape(item),  area.getSpace(), point, facing);
	m_designations.emplace(point, area, items.getReference(item), point, facing, faction, occupied);
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
