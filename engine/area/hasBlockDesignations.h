#pragma once

#include "../types.h"
#include "../config.h"
#include "../designations.h"
#include "idTypes.h"
#include "vectorContainers.h"
#include <unordered_map>

class Area;
struct DeserializationMemo;
class AreaHasBlockDesignationsForFaction final
{
	std::vector<bool> m_designations;
	BlockIndex m_areaSize;
	uint32_t getIndex(const BlockIndex& index, const BlockDesignation designation) const;
public:
	AreaHasBlockDesignationsForFaction(Area& area);
	AreaHasBlockDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo);
	AreaHasBlockDesignationsForFaction(AreaHasBlockDesignationsForFaction&& o) noexcept : m_designations(std::move(o.m_designations)), m_areaSize(o.m_areaSize) { }
	AreaHasBlockDesignationsForFaction& operator=(const AreaHasBlockDesignationsForFaction&& o) { m_designations = std::move(o.m_designations); m_areaSize = o.m_areaSize; return *this; }
	void set(const BlockIndex& index, const BlockDesignation designation);
	void unset(const BlockIndex& index, const BlockDesignation designation);
	void maybeUnset(const BlockIndex& index, const BlockDesignation designation);
	void maybeSet(const BlockIndex& index, const BlockDesignation designation);
	[[nodiscard]] bool check(const BlockIndex& index, const BlockDesignation designation) const;
	[[nodiscard]] BlockDesignation getDisplayDesignation(const BlockIndex& index) const;
	[[nodiscard]] bool empty(const BlockIndex& index) const;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] uint32_t getOffsetForDesignation(const BlockDesignation designation) const { return (uint)designation * m_areaSize.get(); }
	[[nodiscard]] bool checkWithOffset(uint32_t offset, const BlockIndex& index) const { return m_designations[offset + index.get()]; }
	AreaHasBlockDesignationsForFaction(AreaHasBlockDesignationsForFaction&) = delete;
};
class AreaHasBlockDesignations final
{
	Area& m_area;
	SmallMap<FactionId, AreaHasBlockDesignationsForFaction> m_data;
public:
	AreaHasBlockDesignations(Area& area) : m_area(area) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void registerFaction(FactionId faction) { m_data.emplace(faction, m_area); }
	void unregisterFaction(FactionId faction) { m_data.erase(faction); }
	[[nodiscard]] bool contains(FactionId faction) { return m_data.contains(faction); }
	[[nodiscard]] AreaHasBlockDesignationsForFaction& getForFaction(FactionId faction) { return m_data[faction]; }
	[[nodiscard]] const AreaHasBlockDesignationsForFaction& getForFaction(FactionId faction) const { return m_data[faction]; }
	[[nodiscard]] Json toJson() const;
	AreaHasBlockDesignations(AreaHasBlockDesignationsForFaction&) = delete;
	AreaHasBlockDesignations(AreaHasBlockDesignationsForFaction&&) = delete;
};
