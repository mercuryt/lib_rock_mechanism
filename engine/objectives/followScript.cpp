#include "followScript.h"
#include "../area/area.h"
#include "../actors/actors.h"
FollowScriptSubObjective::FollowScriptSubObjective(Objective& parent, const ActorReference& parentOwner) :
	Objective(parent.m_priority),
	m_parent(&parent),
	m_parentOwner(parentOwner)
{ }
FollowScriptSubObjective::FollowScriptSubObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_parent(deserializationMemo.m_objectives[data["parent"]]),
	m_parentOwner(data["parentOwner"], area.getActors().m_referenceData)
{ }
void FollowScriptSubObjective::cancel(Area& area, const ActorIndex& actor) { m_parent->actorGoesOffScript(area, m_parentOwner.getIndex(area.getActors().m_referenceData), actor); }
void FollowScriptSubObjective::delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void FollowScriptSubObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
Json FollowScriptSubObjective::toJson() const
{
	Json data = Objective::toJson();
	data["parentOwner"] = m_parentOwner;
	data["parent"] = m_parent;
	return data;
}