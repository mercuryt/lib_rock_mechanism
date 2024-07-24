#pragma once
#include "../config.h"
#include "../blockIndices.h"
struct Faction;
class DeserializationMemo;
class Area;
class AreaHasSleepingSpots final
{
	Area& m_area;
	BlockIndices m_unassigned;
public:
	AreaHasSleepingSpots(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void designate(FactionId faction, BlockIndex block);
	void undesignate(FactionId faction, BlockIndex block);
	bool containsUnassigned(BlockIndex block) const { return m_unassigned.contains(block); }
};
