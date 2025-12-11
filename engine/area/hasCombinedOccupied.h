#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../dataStructures/smallMap.h"
#include "../numericTypes/idTypes.h"

class AreaHasCombinedOccupied
{
	SmallMap<FactionId, RTreeBoolean> m_data;
public:
	void record(const ActorIndex& actor, Area& area);
	void remove(const ActorIndex& actor, Area& area);
	[[nodiscard]] bool queryAnyEnemyInLineOfSight(const ActorIndex& actor, Area& area) const;
};