#pragma once
#include "../dataStructures/smallMap.h"
#include "../dataStructures/strongVector.h"
#include "../numericTypes/index.h"
#include "../numericTypes/idTypes.h"
#include "../squad.h"
class SimulationHasSquads final
{
	SmallMap<FactionId, StrongVector<Squad, SquadIndex>> m_squads;
public:
	void destroy(const FactionId& faction, const SquadIndex& squadIndex, Simulation& simulation);
	[[nodiscard]] SquadIndex create(const ActorId& commanderId, const FactionId& faction, const std::string& name);
	[[nodiscard]] StrongVector<Squad, SquadIndex>& squadsForFaction(const FactionId& faction);
	[[nodiscard]] const StrongVector<Squad, SquadIndex>& squadsForFaction(const FactionId& faction) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasSquads, m_squads);
};