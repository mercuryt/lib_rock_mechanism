// Record ponters to fire data for use by UI.
#include "space.h"
#include "../definitions/materialType.h"
#include "../fire.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
#include <algorithm>
void Space::fire_maybeIgnite(const Point3D& point, const MaterialTypeId& materialType)
{
	if(!fire_existsForMaterialType(point, materialType))
		m_area.m_fires.ignite(m_area, point, materialType);
}
void Space::fire_setPointer(const Point3D& point, SmallMap<MaterialTypeId, Fire>* pointer)
{
	assert(!m_fires.queryAny(point));
	m_fires.insert(point, PointHasFires(pointer));
}
void Space::fire_clearPointer(const Point3D& point)
{
	assert(m_fires.queryAny(point));
	m_fires.remove(point);
}
bool Space::fire_exists(const Point3D& point) const
{
	return m_fires.queryAny(point);
}
bool Space::fire_existsForMaterialType(const Point3D& point, const MaterialTypeId& materialType) const
{
	const auto ptr = m_fires.queryGetOneOr(point, PointHasFires(nullptr));
	if(ptr.get() == nullptr)
		return false;
	return ptr.get()->contains(materialType);
}
FireStage Space::fire_getStage(const Point3D& point) const
{
	assert(fire_exists(point));
	FireStage output = FireStage::Smouldering;
	for(auto& pair : *(m_fires.queryGetOne(point).get()))
	{
		const auto& stage = pair.second.m_stage;
		if(stage > output)
			output = stage;
	}
	return output;
}
Fire& Space::fire_get(const Point3D& point, const MaterialTypeId& materialType)
{
	return (*m_fires.queryGetOne(point).get())[materialType];
}
