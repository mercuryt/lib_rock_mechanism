#include "pointFeature.h"
const PointFeatureType& PointFeatureType::byName(const std::string& name)
{
	{
		for(const PointFeatureType& type : pointFeatureTypeData)
			if(type.name == name)
				return type;
		std::unreachable();
	}
}
const PointFeatureType& PointFeatureType::byId(const PointFeatureTypeId& id)
{
	assert(id != PointFeatureTypeId::Null);
	return pointFeatureTypeData[(uint)id];
}
void PointFeatureSet::insert(const PointFeatureTypeId& typeId, const MaterialTypeId& materialType, bool hewn, bool closed, bool locked)
{
	assert(!contains(typeId));
	m_data.emplace_back(typeId, materialType, hewn, closed, locked);
}
void PointFeatureSet::remove(const PointFeatureTypeId& typeId)
{
	auto found = std::ranges::find(m_data, typeId, &PointFeature::pointFeatureTypeId);
	assert(found != m_data.end());
	std::swap(m_data.back(), *found);
	m_data.pop_back();
}
bool PointFeatureSet::contains(const PointFeatureTypeId& typeId) const
{
	return std::ranges::contains(m_data, typeId, &PointFeature::pointFeatureTypeId);
}
PointFeature& PointFeatureSet::get(const PointFeatureTypeId& typeId)
{
	auto found = std::ranges::find(m_data, typeId, &PointFeature::pointFeatureTypeId);
	assert(found != m_data.end());
	return *found;
}
PointFeature* PointFeatureSet::maybeGet(const PointFeatureTypeId& typeId)
{
	auto found = std::ranges::find(m_data, typeId, &PointFeature::pointFeatureTypeId);
	if(found != m_data.end())
		return &*found;
	else
		return nullptr;
}
bool PointFeatureSet::canStandIn() const
{
	for(const PointFeature& feature : m_data)
		if(PointFeatureType::byId(feature.pointFeatureTypeId).canStandIn)
			return true;
	return false;
}
bool PointFeatureSet::canStandAbove() const
{
	for(const PointFeature& feature : m_data)
		if(PointFeatureType::byId(feature.pointFeatureTypeId).canStandAbove)
			return true;
	return false;
}
bool PointFeatureSet::blocksEntrance() const
{
	for(const PointFeature& feature : m_data)
		if(PointFeatureType::byId(feature.pointFeatureTypeId).blocksEntrance)
			return true;
	return false;
}
bool PointFeatureSet::isOpaque() const
{
	for(const PointFeature& feature : m_data)
		if(PointFeatureType::byId(feature.pointFeatureTypeId).pointIsOpaque && !MaterialType::getTransparent(feature.materialType))
			return true;
	return false;
}
bool PointFeatureSet::floorIsOpaque() const
{
	for(const PointFeature& feature : m_data)
	{
		const auto& id = feature.pointFeatureTypeId;
		if((id == PointFeatureTypeId::Floor || (id == PointFeatureTypeId::Hatch && feature.closed)) && !MaterialType::getTransparent(feature.materialType))
			return true;
	}
	return false;
}