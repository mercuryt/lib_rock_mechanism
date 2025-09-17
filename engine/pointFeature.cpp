#include "pointFeature.h"
void PointFeature::clear()
{
	materialType.clear();
	pointFeatureType = PointFeatureTypeId::Null;
	hewnAndClosedAndLocked.clear();
}
const PointFeatureType& PointFeatureType::byName(const std::string& name)
{
	for(const PointFeatureType& type : pointFeatureTypeData)
		if(type.name == name)
			return type;
	std::unreachable();
}
const PointFeatureType& PointFeatureType::byId(const PointFeatureTypeId& id)
{
	assert(id != PointFeatureTypeId::Null);
	return pointFeatureTypeData[(uint)id];
}
std::string PointFeature::toString() const
{
	return "{materialType: " + MaterialType::getName(materialType) + ", : pointFeatureType:" + PointFeatureType::byId(pointFeatureType).name + ", hewn: " + std::to_string(isHewn()) + ", closed: " + std::to_string(isClosed()) + ", locked: " + std::to_string(isLocked()) + "}";
}