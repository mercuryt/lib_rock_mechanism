#pragma once
#include "../onSight.h"
#include "../numericTypes/idTypes.h"
#include "../dataStructures/smallMap.h"

class AreaHasOnSightForFaction
{
	SmallMap<FactionId, HasOnSight> m_data;
public:
	AreaHasOnSightForFaction() = default;
	AreaHasOnSightForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void maybeExecute(const FactionId& faction, Area& area, const ActorReference& canSee, const SmallSet<ActorReference>& canNowBeSeen);
	void maybeExecute(const FactionId& faction, Area& area, const ActorReference& canSee, const ActorReference& canNowBeSeen);
	[[nodiscard]] HasOnSight& get(const FactionId& faction);
	[[nodiscard]] const HasOnSight& get(const FactionId& faction) const;
	[[nodiscard]] Json toJson() const;
};