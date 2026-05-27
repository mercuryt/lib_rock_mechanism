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
	return pointFeatureTypeData[(int)id];
}
std::vector<PointFeatureType*> PointFeatureType::getAll()
{
	std::vector<PointFeatureType*> output;
	output.reserve(pointFeatureTypeData.size());
	for(PointFeatureType& type : pointFeatureTypeData)
		output.push_back(&type);
	return output;
}
PointFeatureTypeId PointFeatureType::getId(const PointFeatureType& type)
{
	return PointFeatureTypeId(&type - pointFeatureTypeData.begin());
}
std::string PointFeature::toString() const
{
	return "{materialType: " + MaterialType::getName(materialType) + ", : pointFeatureType:" + PointFeatureType::byId(pointFeatureType).name + ", hewn: " + std::to_string(isHewn()) + ", closed: " + std::to_string(isClosed()) + ", locked: " + std::to_string(isLocked()) + "}";
}
bool PointFeature::blocksLineOfSight() const
{
	return !MaterialType::getTransparent(materialType) && PointFeatureType::byId(pointFeatureType).opaque && isClosed();
}
bool PointFeature::blocksVerticalTravel() const
{
	return pointFeatureType == PointFeatureTypeId::Floor || pointFeatureType == PointFeatureTypeId::FloorGrate || (pointFeatureType == PointFeatureTypeId::Hatch && isLocked());
}
bool PointFeature::blocksVerticalTravelEver() const
{
	return pointFeatureType == PointFeatureTypeId::Floor || pointFeatureType == PointFeatureTypeId::FloorGrate || pointFeatureType == PointFeatureTypeId::Hatch;
}
bool PointFeature::blocksEntrance() const
{
	return PointFeatureType::byId(pointFeatureType).blocksEntrance || (pointFeatureType == PointFeatureTypeId::Door && isLocked());
}

