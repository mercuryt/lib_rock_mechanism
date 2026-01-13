/*
	Positive Malice is friendly and negitive Malice is enemy.
	TODO: allies.
	TODO: It is possible for an actor with negitive courage and low combat score to have to do a courage test against nothing. Cheper to prevent or to let it happen?
*/
#pragma once
#include "../dataStructures/smallMap.h"
#include "../dataStructures/rtreeData.h"
#include "../numericTypes/idTypes.h"
#include "../numericTypes/types.h"
class Area;
class AreaHasSoldiers;
// Create one or more threads per faction.
struct AreaHasSoldiersCourageCheckThreadData
{
	const FactionId faction;
	const int8_t start;
	void prefetchToL1(const AreaHasSoldiers& areaHasSoldiers) const;
};
struct AreaHasSoldiersForFaction
{
	RTreeData<PsycologyWeight> maliceMap;
	std::vector<ActorIndex> soldiers;
	std::vector<Point3D> soldierLocations;
	std::vector<PsycologyWeight> courage;
	void prefetchToL3() const;
};
class AreaHasSoldiers
{
	SmallMap<FactionId, AreaHasSoldiersForFaction> m_data;
public:
	void add(Area& area, const ActorIndex& actor);
	void remove(Area& area, const ActorIndex& actor);
	void setLocation(Area& area, const ActorIndex& actor);
	// To be used by updateSoldierCombatScore.
	void unsetLocation(Area& area, const ActorIndex& actor, const CombatScore& combatScore);
	void unsetLocation(Area& area, const ActorIndex& actor);
	void updateLocation(Area& area, const ActorIndex& actor);
	void updateSoldierIndex(Area& area, const ActorIndex& oldIndex, const ActorIndex& newIndex);
	void updateSoldierCourage(Area& area, const ActorIndex& actor);
	void updateSoldierCombatScore(Area& area, const ActorIndex& actor, const CombatScore& previous);
	void doStepThread(Area& area, const AreaHasSoldiersCourageCheckThreadData& threadData);
	void doStep(Area& area);
	[[nodiscard]] PsycologyWeight get(const Point3D& point, const FactionId& faction) const;
	friend struct AreaHasSoldiersCourageCheckThreadData;
};