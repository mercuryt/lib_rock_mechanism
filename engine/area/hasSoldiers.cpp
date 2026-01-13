#include "hasSoldiers.h"
#include "area.h"
#include "../actors/actors.h"
#include "../config/psycology.h"
#include "../dataStructures/rtreeData.hpp"
void AreaHasSoldiersForFaction::prefetchToL3() const
{
	util::prefetchL3ReadMode(soldiers);
	util::prefetchL3ReadMode(soldierLocations);
	util::prefetchL3ReadMode(courage);
}
void AreaHasSoldiersCourageCheckThreadData::prefetchToL1(const AreaHasSoldiers& areaHasSoldiers) const
{
	auto& forFaction = areaHasSoldiers.m_data[faction];
	int32_t count = std::min(Config::soldiersPerMoraleCheckThread, forFaction.soldiers.size());
	util::prefetchL1ReadModeSegment(forFaction.soldiers, start, count);
	util::prefetchL1ReadModeSegment(forFaction.soldierLocations, start, count);
	util::prefetchL1ReadModeSegment(forFaction.courage, start, count);
}
void AreaHasSoldiers::add(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	auto& forFaction = m_data[faction];
	//TODO: getFaction is run twice.
	setLocation(area, actor);
	forFaction.soldiers.push_back(actor);
	forFaction.courage.push_back(actors.psycology_get(actor).getValueFor(PsycologyAttribute::Courage));
}
void AreaHasSoldiers::remove(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	auto& forFaction = m_data[faction];
	//TODO: getFaction is run twice.
	unsetLocation(area, actor);
	auto& soldiers = forFaction.soldiers;
	auto found = std::ranges::find(soldiers, actor);
	auto index = found - soldiers.begin();
	(*found) = soldiers.back();
	soldiers.pop_back();
	auto& courage = forFaction.courage;
	courage[index] = courage.back();
	courage.pop_back();
}
void AreaHasSoldiers::setLocation(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	const CuboidSet& malicePoints = actors.combat_makeMalicePoints(actor);
	const CombatScore& combatScore = actors.combat_getCombatScore(actor);
	const PsycologyWeight combatScoreAsWeight{(float)combatScore.get()};
	actors.soldier_getRecordedMalicePoints(actor) = malicePoints;
	auto& forFaction = m_data[faction];
	forFaction.maliceMap.updateAdd(malicePoints, combatScoreAsWeight);
	auto& soldiers = forFaction.soldiers;
	auto found = std::ranges::find(soldiers, actor);
	auto index = found - soldiers.begin();
	forFaction.soldierLocations[index] = actors.getLocation(actor);
	for(const FactionId& enemy : area.m_simulation.m_hasFactions.getEnemies(faction))
		m_data[enemy].maliceMap.updateSubtract(malicePoints, combatScoreAsWeight);
}
void AreaHasSoldiers::unsetLocation(Area& area, const ActorIndex& actor)
{
	const CombatScore& combatScore = area.getActors().combat_getCombatScore(actor);
	unsetLocation(area, actor, combatScore);
}
void AreaHasSoldiers::unsetLocation(Area& area, const ActorIndex& actor, const CombatScore& combatScore)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	const PsycologyWeight combatScoreAsWeight{(float)combatScore.get()};
	auto& forFaction = m_data[faction];
	CuboidSet& malicePoints = actors.soldier_getRecordedMalicePoints(actor);
	for(const FactionId& enemy : area.m_simulation.m_hasFactions.getEnemies(faction))
		m_data[enemy].maliceMap.updateAdd(malicePoints, combatScoreAsWeight);
	forFaction.maliceMap.updateSubtract(malicePoints, combatScoreAsWeight);
	actors.soldier_getRecordedMalicePoints(actor).clear();
	auto& soldiers = forFaction.soldiers;
	auto found = std::ranges::find(soldiers, actor);
	assert(found != soldiers.end());
	auto index = found - soldiers.begin();
	auto& locations = forFaction.soldierLocations;
	locations[index].clear();
}
void AreaHasSoldiers::updateLocation(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	CuboidSet& previousMalicePoints =  actors.soldier_getRecordedMalicePoints(actor);
	//TODO: getMalicePoints should be moved to soldier?
	const CuboidSet newMalicePoints = actors.combat_makeMalicePoints(actor);
	const FactionId& faction = actors.getFaction(actor);
	auto& forFaction = m_data[faction];
	const CombatScore& combatScore = actors.combat_getCombatScore(actor);
	const PsycologyWeight combatScoreAsWeight{(float)combatScore.get()};
	forFaction.maliceMap.updateSubtract(previousMalicePoints, combatScoreAsWeight);
	forFaction.maliceMap.updateAdd(newMalicePoints, combatScoreAsWeight);
	auto& soldiers = forFaction.soldiers;
	auto found = std::ranges::find(soldiers, actor);
	auto index = found - soldiers.begin();
	auto& locations = forFaction.soldierLocations;
	locations[index] = actors.getLocation(actor);
	for(const FactionId& enemy : area.m_simulation.m_hasFactions.getEnemies(faction))
	{
		m_data[enemy].maliceMap.updateAdd(previousMalicePoints, combatScoreAsWeight);
		m_data[enemy].maliceMap.updateSubtract(newMalicePoints, combatScoreAsWeight);
	}
	previousMalicePoints = newMalicePoints;
}
PsycologyWeight AreaHasSoldiers::get(const Point3D& point, const FactionId& faction) const
{
	return m_data[faction].maliceMap.queryGetOne(point);
}
void AreaHasSoldiers::updateSoldierIndex(Area& area, const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(newIndex);
	auto& forFaction = m_data[faction];
	auto& soldiers = forFaction.soldiers;
	auto found = std::ranges::find(soldiers, oldIndex);
	(*found) = newIndex;
}
void AreaHasSoldiers::updateSoldierCourage(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	auto& forFaction = m_data[faction];
	auto& soldiers = forFaction.soldiers;
	auto found = std::ranges::find(soldiers, actor);
	auto index = found - soldiers.begin();
	PsycologyWeight courage = actors.psycology_get(actor).getValueFor(PsycologyAttribute::Courage);
	forFaction.courage[index] = {courage.get()};
}
void AreaHasSoldiers::updateSoldierCombatScore(Area& area, const ActorIndex& actor, const CombatScore& previous)
{
	// TODO: redundant lookups and soldierLocation setting.
	unsetLocation(area, actor, previous);
	setLocation(area, actor);
}
void AreaHasSoldiers::doStepThread(Area& area, const AreaHasSoldiersCourageCheckThreadData& threadData)
{
	threadData.prefetchToL1(*this);
	SmallMap<int32_t, PsycologyWeight> actorsNeedingToTestCourage;
	auto& forFaction = m_data[threadData.faction];
	const auto& courage = forFaction.courage;
	const auto& soldiers = forFaction.soldiers;
	std::vector<Point3D> soldierLocationsForThisThread;
	const int8_t end = forFaction.soldierLocations.size() - threadData.start > Config::soldiersPerMoraleCheckThread ? threadData.start + Config::soldiersPerMoraleCheckThread : forFaction.soldierLocations.size();
	// Wolud it be better to pass a view rather then copy the soldiers locations into a smaller vector?
	for(int i = threadData.start; i != end; ++i)
		soldierLocationsForThisThread.push_back(forFaction.soldierLocations[i]);
	forFaction.maliceMap.batchQueryForEach(soldierLocationsForThisThread, [&](const PsycologyWeight maliceDelta, const Cuboid&, const int32_t& index){
		const int32_t adjustedIndex = index + threadData.start;
		if(maliceDelta + courage[adjustedIndex] < Config::Psycology::minimumMaliceDeltaPlusCourageToHoldFirmWithoutTest)
			actorsNeedingToTestCourage.insert(adjustedIndex, maliceDelta);
	});
	Actors& actors = area.getActors();
	for(const auto& [index, maliceDelta] : actorsNeedingToTestCourage)
	{
		bool holdsFirm = area.m_simulation.m_random.applyRandomFuzzPlusOrMinusRatio(courage[index], Config::ratioOfMaximumVarianceForCourageTest) > maliceDelta;
		if(holdsFirm)
			// Test of courage passed, hold ground and gain permanant courage.
			actors.psycology_event(soldiers[index], PsycologyEventType::StandGround, PsycologyAttribute::Courage, Config::Psycology::courageToGainOnStandGround, Step::null(), Config::Psycology::coolDownForStandGround);
		else
			// Test failed, flee.
			actors.combat_flee(soldiers[index]);
			// TODO: gain shame?
	}
}
void AreaHasSoldiers::doStep(Area& area)
{
	static std::vector<AreaHasSoldiersCourageCheckThreadData> threadDatas;
	if(area.m_simulation.m_step % Config::Psycology::intervalToCheckIfSoldiersFlee == 0)
	{
		threadDatas.clear();
		for(const auto& [faction, forFaction] : m_data)
		{
			int8_t soldiersAssignedToThread = 0;
			forFaction.prefetchToL3();
			int8_t soldierCount = forFaction.soldiers.size();
			while(soldiersAssignedToThread < soldierCount)
			{
				threadDatas.emplace_back(faction, soldiersAssignedToThread);
				soldiersAssignedToThread += Config::soldiersPerMoraleCheckThread;
			}
		}
		// This is safe to parallelize without having read/write steps because the changes are only internal to the actors.
		// If that stops being true, for example if combat_isFleeing changed combatScore, then this would be a race condition.
		#pragma omp parallel for
			for(const AreaHasSoldiersCourageCheckThreadData& threadData : threadDatas)
				doStepThread(area, threadData);
	}
}