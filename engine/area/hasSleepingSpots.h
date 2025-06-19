#pragma once
#include "../config.h"
#include "../index.h"
struct Faction;
struct DeserializationMemo;
class Area;
class AreaHasSleepingSpots final
{
	Area& m_area;
	SmallSet<BlockIndex> m_unassigned;
public:
	AreaHasSleepingSpots(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void designate(const FactionId& faction, const BlockIndex& block);
	void undesignate(const FactionId& faction, const BlockIndex& block);
	bool containsUnassigned(const BlockIndex& block) const { return m_unassigned.contains(block); }
};
