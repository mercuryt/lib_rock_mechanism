#include "actors.h"
#include "../area/area.h"

void Actors::soldier_removeFromMaliceMap(const ActorIndex& index)
{
	m_area.m_maliceMap[m_faction[index]].updateSubtract(m_soldier[index].maliceZone, m_combatScore[index]);
	// No need to clear maliceZone, it get's overrwritten before each use.
}
void Actors::soldier_recordInMaliceMap(const ActorIndex& index)
{
	m_soldier[index].maliceZone = combat_getMaliceZone(index);
	m_area.m_maliceMap[m_faction[index]].updateAdd(m_soldier[index].maliceZone, m_combatScore[index]);
}
void Actors::soldier_mobilize(const ActorIndex& index)
{
	m_soldier[index].isDrafted = true;
	soldier_recordInMaliceMap(index);
}
void Actors::soldier_demobilize(const ActorIndex& index)
{
	soldier_removeFromMaliceMap(index);
	m_soldier[index].isDrafted = false;
}
CombatScore Actors::soldier_getEnemyMaliceAtLocation(const ActorIndex& index, const Point3D& location) const
{
	CombatScore output = {0};
	const FactionId& soldierFaction = m_faction[index];
	for(const auto& [factionId, maliceMap] : m_area.m_maliceMap)
		if(m_area.m_simulation.m_hasFactions.isEnemy(factionId, soldierFaction))
			output += maliceMap.queryGetOne(location);
	return output;
}
CombatScore Actors::soldier_getFriendlyMaliceAtLocation(const ActorIndex& index, const Point3D& location) const
{
	return m_area.m_maliceMap[m_faction[index]].queryGetOne(location);
}
bool Actors::soldier_testMaliceDeltaAtLocationAgainstCourage(const ActorIndex& index, const Point3D& location) const
{
	int32_t friendly = (int32_t)soldier_getFriendlyMaliceAtLocation(index, location).get();
	int32_t enemy =  (int32_t)soldier_getEnemyMaliceAtLocation(index, location).get();
	// High delta is more threat.
	int32_t delta = enemy - friendly;
	const PsycologyWeight& courage = psycology_getConst(index).getValueFor(PsycologyAttribute::Courage);
	// Zero courage gives an even chance when there is no malice delta.
	return m_area.m_simulation.m_random.applyRandomFuzzPlusOrMinusRatio(courage, Config::ratioOfMaximumVarianceForCourageTest) > delta;
}
bool Actors::soldier_is(const ActorIndex& index) const { return m_soldier[index].squad.exists(); }