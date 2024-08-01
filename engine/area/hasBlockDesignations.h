#pragma once

#include "../types.h"
#include "../config.h"
#include "../lib/dynamic_bitset.hpp"
#include "../designations.h"

class Area;
struct DeserializationMemo;
class AreaHasBlockDesignationsForFaction final
{
	Area& m_area;
	sul::dynamic_bitset<> m_designations;
	int getIndex(BlockIndex index, BlockDesignation designation) const;
public:
	AreaHasBlockDesignationsForFaction(Area& area);
	AreaHasBlockDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void set(BlockIndex index, BlockDesignation designation);
	void unset(BlockIndex index, BlockDesignation designation);
	void maybeUnset(BlockIndex index, BlockDesignation designation);
	void maybeSet(BlockIndex index, BlockDesignation designation);
	[[nodiscard]] bool check(BlockIndex index, BlockDesignation designation) const;
	[[nodiscard]] BlockDesignation getDisplayDesignation(BlockIndex index) const;
	[[nodiscard]] bool empty(BlockIndex) const;
	[[nodiscard]] Json toJson() const;
	AreaHasBlockDesignationsForFaction(AreaHasBlockDesignationsForFaction&) = delete;
	AreaHasBlockDesignationsForFaction(AreaHasBlockDesignationsForFaction&&) = delete;
};
class AreaHasBlockDesignations final
{
	Area& m_area;
	FactionIdMap<AreaHasBlockDesignationsForFaction> m_data;
public:
	AreaHasBlockDesignations(Area& area) : m_area(area) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void registerFaction(FactionId faction) { m_data.emplace(faction, m_area); }
	void unregisterFaction(FactionId faction) { m_data.erase(faction); }
	[[nodiscard]] bool contains(FactionId faction) { return m_data.contains(faction); }
	[[nodiscard]] AreaHasBlockDesignationsForFaction& at(FactionId faction) { return m_data.at(faction); }
	[[nodiscard]] const AreaHasBlockDesignationsForFaction& at(FactionId faction) const { return m_data.at(faction); }
	[[nodiscard]] Json toJson() const;
	AreaHasBlockDesignations(AreaHasBlockDesignationsForFaction&) = delete;
	AreaHasBlockDesignations(AreaHasBlockDesignationsForFaction&&) = delete;
};
