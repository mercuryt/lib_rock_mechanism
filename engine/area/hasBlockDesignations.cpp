#include "hasBlockDesignations.h"
#include "../area/area.h"
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
uint32_t AreaHasBlockDesignationsForFaction::getIndex(const BlockIndex& index, const BlockDesignation designation) const
{
	return (m_areaSize.get() * (uint)designation) + index.get();
}
void AreaHasBlockDesignationsForFaction::set(const BlockIndex& index, const BlockDesignation designation)
{
	assert(index.exists());
	assert(!m_designations[getIndex(index, designation)]);
	m_designations[getIndex(index, designation)] = true;
}
void AreaHasBlockDesignationsForFaction::unset(const BlockIndex& index, const BlockDesignation designation)
{
	assert(m_designations[getIndex(index, designation)]);
	m_designations[getIndex(index, designation)] = false;
}
void AreaHasBlockDesignationsForFaction::maybeSet(const BlockIndex& index, const BlockDesignation designation)
{
	m_designations[getIndex(index, designation)] = true;
}
void AreaHasBlockDesignationsForFaction::maybeUnset(const BlockIndex& index, const BlockDesignation designation)
{
	m_designations[getIndex(index, designation)] = false;
}
bool AreaHasBlockDesignationsForFaction::check(const BlockIndex& index, const BlockDesignation designation) const
{
	return m_designations[getIndex(index, designation)];
}
BlockDesignation AreaHasBlockDesignationsForFaction::getDisplayDesignation(const BlockIndex& index) const
{
	for(int i = 0; i < (int)BlockDesignation::BLOCK_DESIGNATION_MAX; ++i)
		if(check(index, (BlockDesignation)i))
			return (BlockDesignation)i;
	return BlockDesignation::BLOCK_DESIGNATION_MAX;
}
bool AreaHasBlockDesignationsForFaction::empty(const BlockIndex& index) const
{
	for(int i = 0; i < (int)BlockDesignation::BLOCK_DESIGNATION_MAX; ++i)
		if(check(index, (BlockDesignation)i))
			return true;
	return false;
}
std::vector<BlockDesignation> AreaHasBlockDesignationsForFaction::getForBlock(const BlockIndex& index) const
{
	std::vector<BlockDesignation> output;
	output.reserve(uint(BlockDesignation::BLOCK_DESIGNATION_MAX) / 2);
	for(auto designation = BlockDesignation(0); designation != BlockDesignation::BLOCK_DESIGNATION_MAX; designation = BlockDesignation((uint)designation + 1))
		if(check(index, designation))
			output.push_back(designation);
	return output;
}
Json AreaHasBlockDesignationsForFaction::toJson() const
{
	return m_designations;
}
void AreaHasBlockDesignations::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& pair : data)
	{
		const FactionId faction = pair[0].get<FactionId>();
		const Json& designationData = pair[1];
		m_data.emplace(faction, designationData, deserializationMemo);
	}
}
AreaHasBlockDesignationsForFaction& AreaHasBlockDesignations::maybeRegisterAndGetForFaction(const FactionId& faction)
{
	auto found = m_data.find(faction);
	if(found != m_data.end())
		return found.second();
	registerFaction(faction);
	return m_data[faction];
}
const AreaHasBlockDesignationsForFaction& AreaHasBlockDesignations::maybeRegisterAndGetForFaction(const FactionId& faction) const
{
	return const_cast<AreaHasBlockDesignations&>(*this).maybeRegisterAndGetForFaction(faction);
}
std::vector<std::pair<FactionId, BlockDesignation>> AreaHasBlockDesignations::getForBlock(const BlockIndex& block) const
{
	std::vector<std::pair<FactionId, BlockDesignation>> output;
	for(const auto& [faction, forFaction] : m_data)
		for(const auto& designation : forFaction->getForBlock(block))
			output.emplace_back(faction, designation);
	return output;
}
Json AreaHasBlockDesignations::toJson() const
{
	Json output;
	for(const auto& [faction, designations] : m_data)
		output.push_back({faction, designations->toJson()});
	return output;
}