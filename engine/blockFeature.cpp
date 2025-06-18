#include "blockFeature.h"
const BlockFeatureType& BlockFeatureType::byName(const std::string& name)
{
	{
		for(const BlockFeatureType& type : blockFeatureTypeData)
			if(type.name == name)
				return type;
		std::unreachable();
	}
}
const BlockFeatureType& BlockFeatureType::byId(const BlockFeatureTypeId& id)
{
	assert(id != BlockFeatureTypeId::Null);
	return blockFeatureTypeData[(uint)id];
}
void BlockFeatureSet::insert(const BlockFeatureTypeId& typeId, const MaterialTypeId& materialType, bool hewn, bool closed, bool locked)
{
	assert(!contains(typeId));
	m_data.emplace_back(typeId, materialType, hewn, closed, locked);
}
void BlockFeatureSet::remove(const BlockFeatureTypeId& typeId)
{
	auto found = std::ranges::find(m_data, typeId, &BlockFeature::blockFeatureTypeId);
	assert(found != m_data.end());
	std::swap(m_data.back(), *found);
	m_data.pop_back();
}
bool BlockFeatureSet::contains(const BlockFeatureTypeId& typeId) const
{
	return std::ranges::contains(m_data, typeId, &BlockFeature::blockFeatureTypeId);
}
BlockFeature& BlockFeatureSet::get(const BlockFeatureTypeId& typeId)
{
	auto found = std::ranges::find(m_data, typeId, &BlockFeature::blockFeatureTypeId);
	assert(found != m_data.end());
	return *found;
}
BlockFeature* BlockFeatureSet::maybeGet(const BlockFeatureTypeId& typeId)
{
	auto found = std::ranges::find(m_data, typeId, &BlockFeature::blockFeatureTypeId);
	if(found != m_data.end())
		return &*found;
	else
		return nullptr;
}
bool BlockFeatureSet::canStandIn() const
{
	for(const BlockFeature& feature : m_data)
		if(BlockFeatureType::byId(feature.blockFeatureTypeId).canStandIn)
			return true;
	return false;
}
bool BlockFeatureSet::canStandAbove() const
{
	for(const BlockFeature& feature : m_data)
		if(BlockFeatureType::byId(feature.blockFeatureTypeId).canStandAbove)
			return true;
	return false;
}
bool BlockFeatureSet::blocksEntrance() const
{
	for(const BlockFeature& feature : m_data)
		if(BlockFeatureType::byId(feature.blockFeatureTypeId).blocksEntrance)
			return true;
	return false;
}
bool BlockFeatureSet::isOpaque() const
{
	for(const BlockFeature& feature : m_data)
		if(BlockFeatureType::byId(feature.blockFeatureTypeId).blockIsOpaque && !MaterialType::getTransparent(feature.materialType))
			return true;
	return false;
}
bool BlockFeatureSet::floorIsOpaque() const
{
	for(const BlockFeature& feature : m_data)
	{
		const auto& id = feature.blockFeatureTypeId;
		if((id == BlockFeatureTypeId::Floor || (id == BlockFeatureTypeId::Hatch && feature.closed)) && !MaterialType::getTransparent(feature.materialType))
			return true;
	}
	return false;
}