#pragma once
#include "../config.h"
#include <unordered_set>
struct Faction;
class DeserializationMemo;
class Area;
class AreaHasSleepingSpots final
{
	Area& m_area;
	std::unordered_set<BlockIndex> m_unassigned;
public:
	AreaHasSleepingSpots(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void designate(Faction& faction, BlockIndex block);
	void undesignate(Faction& faction, BlockIndex block);
	bool containsUnassigned(BlockIndex block) const { return m_unassigned.contains(block); }
};
