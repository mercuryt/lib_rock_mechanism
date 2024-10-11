#include "hasBlockDesignations.h"
#include "../area.h"
#include "../designations.h"
#include "../json.h"
#include "../deserializationMemo.h"
#include "blocks/blocks.h"
AreaHasBlockDesignationsForFaction::AreaHasBlockDesignationsForFaction(Area& area) : m_areaSize(BlockIndex::create(area.getBlocks().size()))
{
	m_designations.resize(m_areaSize.get() * (uint)BlockDesignation::BLOCK_DESIGNATION_MAX);
}
AreaHasBlockDesignationsForFaction::AreaHasBlockDesignationsForFaction(const Json& data, DeserializationMemo&) : 
	m_designations(data.get<std::vector<bool>>()) { }
int AreaHasBlockDesignationsForFaction::getIndex(BlockIndex index, BlockDesignation designation) const
{
	return (m_areaSize.get() * (uint)designation) + index.get();
}
void AreaHasBlockDesignationsForFaction::set(BlockIndex index, BlockDesignation designation)
{
	assert(!m_designations[getIndex(index, designation)]);
	m_designations[getIndex(index, designation)] = true;
}
void AreaHasBlockDesignationsForFaction::unset(BlockIndex index, BlockDesignation designation)
{
	assert(m_designations[getIndex(index, designation)]);
	m_designations[getIndex(index, designation)] = false;
}
void AreaHasBlockDesignationsForFaction::maybeSet(BlockIndex index, BlockDesignation designation)
{
	m_designations[getIndex(index, designation)] = true;
}
void AreaHasBlockDesignationsForFaction::maybeUnset(BlockIndex index, BlockDesignation designation)
{
	m_designations[getIndex(index, designation)] = false;
}
bool AreaHasBlockDesignationsForFaction::check(BlockIndex index, BlockDesignation designation) const
{
	return m_designations[getIndex(index, designation)];
}
BlockDesignation AreaHasBlockDesignationsForFaction::getDisplayDesignation(BlockIndex index) const
{
	for(int i = 0; i < (int)BlockDesignation::BLOCK_DESIGNATION_MAX; ++i)
		if(check(index, (BlockDesignation)i))
			return (BlockDesignation)i;
	assert(false);
	return BlockDesignation::Construct;
}
bool AreaHasBlockDesignationsForFaction::empty(BlockIndex index) const
{
	for(int i = 0; i < (int)BlockDesignation::BLOCK_DESIGNATION_MAX; ++i)
		if(check(index, (BlockDesignation)i))
			return true;
	return false;
}
Json AreaHasBlockDesignationsForFaction::toJson() const
{
	return m_designations;
}
void AreaHasBlockDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		FactionId faction = pair[0].get<FactionId>();
		const Json& designationData = pair[1];
		m_data.emplace(faction, designationData, deserializationMemo);
	}
}
Json AreaHasBlockDesignations::toJson() const
{
	Json output;
	for(auto& [faction, designations] : m_data)
		output.push_back({faction, designations.toJson()});
	return output;
}
