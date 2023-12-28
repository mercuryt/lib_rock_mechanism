/*
 * Block designations indicate what action a block is suposed to be involved in.
 */
#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "faction.h"
#include <unordered_set>
#include <unordered_map>
#include <cassert>

enum class BlockDesignation { Dig, Construct, SowSeeds, GivePlantFluid, Harvest, StockPileHaulFrom, StockPileHaulTo, Sleep, Rescue, FluidSource };
NLOHMANN_JSON_SERIALIZE_ENUM(BlockDesignation, {
		{BlockDesignation::Dig, "Dig"},
		{BlockDesignation::Construct, "Construct"},
		{BlockDesignation::SowSeeds, "SowSeeds"},
		{BlockDesignation::GivePlantFluid, "GivePlantFluid"},
		{BlockDesignation::Harvest, "Harvest"},
		{BlockDesignation::StockPileHaulFrom, "StockPileHaulFrom"},
		{BlockDesignation::StockPileHaulTo, "StockPileHaulTo"},
		{BlockDesignation::Sleep, "Sleep"},
		{BlockDesignation::Rescue, "Rescue"},
		{BlockDesignation::FluidSource , "FluidSource"}});
class HasDesignations
{
	std::unordered_map<const Faction*, std::unordered_set<BlockDesignation>> m_designations;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo)
	{
		for(const Json& pair : data)
		{
			const Faction& faction = deserializationMemo.faction(pair[0].get<std::wstring>());
			for(const Json& designation : pair[1])
				m_designations[&faction].insert(designation.get<BlockDesignation>());
		}
	}
	Json toJson() const
	{
		Json data;
		for(auto pair : m_designations)
		{
			Json jsonPair{pair.first->m_name, Json::array()};
			for(BlockDesignation designation : pair.second)
				jsonPair[1].push_back(designation);
		}
		return data;
	}
	bool contains(const Faction& f, const BlockDesignation& bd) const 
	{ 
		if(!m_designations.contains(&f))
			return false;
		return m_designations.at(&f).contains(bd); 
	}
	bool empty() const 
	{
		return m_designations.empty();
	}
	void insert(const Faction& f, const BlockDesignation& bd)
	{
		assert(!m_designations[&f].contains(bd));
	       	m_designations[&f].insert(bd);
       	}
	void remove(const Faction& f, const BlockDesignation& bd)
	{
		assert(m_designations[&f].contains(bd));
		if(m_designations[&f].size() == 1)
			m_designations.erase(&f);
		else
	      		m_designations[&f].erase(bd);
	}
	void removeIfExists(const Faction& f, const BlockDesignation& bd) { m_designations[&f].erase(bd); }
};
