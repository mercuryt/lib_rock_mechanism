#include "inspirationalSpeach.h"
#include "../../area/area.h"
#include "../../actors/actors.h"
#include "../../config/social.h"
#include "../../objectives/inspirationalSpeach.h"
InspirationalSpeachDramaArc::InspirationalSpeachDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine) :
	DramaArc(data, deserializationMemo, dramaEngine)
{
	data["cooldowns"].get_to(m_cooldowns);
}
Json InspirationalSpeachDramaArc::toJson() const
{
	Json output = DramaArc::toJson();
	output["cooldowns"] = m_cooldowns;
	return output;
}
bool InspirationalSpeachDramaArc::ready(const FactionId& faction) const { return m_cooldowns[faction] < m_area->m_simulation.m_step; }
void InspirationalSpeachDramaArc::begin(const ActorIndex& actor)
{
	Actors& actors = m_area->getActors();
	const FactionId& faction = actors.getFaction(actor);
	assert(ready(faction));
	m_cooldowns[faction] = m_area->m_simulation.m_step + Config::Social::inspirationalSpeachCoolDownDuration;
	actors.objective_addTaskToStart(actor, std::unique_ptr<InspirationalSpeachObjective>());
}