#include "hasOnSightForFaction.h"
AreaHasOnSightForFaction::AreaHasOnSightForFaction(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	for(const Json& pair : data["data"])
	{
		auto faction = data["data"][0].get<FactionId>();
		m_data.emplace(faction, pair[1], deserializationMemo, area);
	}
}
void AreaHasOnSightForFaction::maybeExecute(const FactionId& faction, Area& area, const ActorReference& canSee, const SmallSet<ActorReference>& canNowBeSeen)
{
	if(canNowBeSeen.empty())
		return;
	auto found = m_data.find(faction);
	if(found == m_data.end())
		return;
	found->second.execute(area, canSee, canNowBeSeen);
}
void AreaHasOnSightForFaction::maybeExecute(const FactionId& faction, Area& area, const ActorReference& canSee, const ActorReference& canNowBeSeen)
{
	auto found = m_data.find(faction);
	if(found == m_data.end())
		return;
	found->second.execute(area, canSee, canNowBeSeen);
}
HasOnSight& AreaHasOnSightForFaction::get(const FactionId& faction) { return m_data[faction]; }
const HasOnSight& AreaHasOnSightForFaction::get(const FactionId& faction) const { return m_data[faction]; }
Json AreaHasOnSightForFaction::toJson() const
{
	Json output{{"data", Json::array()}};
	for(const auto& [faction, onSight] : m_data)
		output.push_back(std::make_pair(faction, onSight.toJson()));
	return output;
}