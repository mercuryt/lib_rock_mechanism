#pragma once

#include "../types.h"
#include "../config.h"
#include "../designations.h"
#include "idTypes.h"
#include <unordered_map>

class Area;
struct DeserializationMemo;
class AreaHasBlockDesignationsForFaction final
{
	Area& m_area;
	std::vector<bool> m_designations;
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
	std::unordered_map<FactionId, AreaHasBlockDesignationsForFaction, FactionId::Hash> m_data;
public:
	AreaHasBlockDesignations(Area& area) : m_area(area) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void registerFaction(FactionId faction) { m_data.emplace(faction, m_area); }
	void unregisterFaction(FactionId faction) { m_data.erase(faction); }
	[[nodiscard]] bool contains(FactionId faction) { return m_data.contains(faction); }
	[[nodiscard]] AreaHasBlockDesignationsForFaction& getForFaction(FactionId faction) { return m_data.at(faction); }
	[[nodiscard]] const AreaHasBlockDesignationsForFaction& getForFaction(FactionId faction) const { return m_data.at(faction); }
	[[nodiscard]] Json toJson() const;
	AreaHasBlockDesignations(AreaHasBlockDesignationsForFaction&) = delete;
	AreaHasBlockDesignations(AreaHasBlockDesignationsForFaction&&) = delete;
};
