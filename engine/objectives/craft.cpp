#include "craft.h"
#include "../skillType.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../area.h"
#include "../terrainFacade.hpp"
CraftPathRequest::CraftPathRequest(Area& area, CraftObjective& co, const ActorIndex& actorIndex) :
	m_craftObjective(co)
{
	assert(m_craftObjective.m_craftJob == nullptr);
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	// TODO: Change this name.
	maxRange = Config::maxRangeToSearchForCraftingRequirements;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_craftObjective.m_detour;
	adjacent = true;
	reserveDestination = true;
}
CraftPathRequest::CraftPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequestBreadthFirst(data, area),
	m_craftObjective(static_cast<CraftObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))),
	m_craftJob(deserializationMemo.m_craftJobs.at(data["craftJob"].get<uintptr_t>())),
	m_location(data["location"].get<BlockIndex>()) { }
FindPathResult CraftPathRequest::readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	FactionId faction = actors.getFactionId(actorIndex);
	HasCraftingLocationsAndJobsForFaction& hasCrafting = area.m_hasCraftingLocationsAndJobs.getForFaction(faction);
	Blocks& blocks = area.getBlocks();
	SkillTypeId skillType = m_craftObjective.m_skillType;
	auto& excludeJobs = m_craftObjective.getFailedJobs();
	auto predicate = [&blocks, faction, &hasCrafting, actorIndex, skillType, &excludeJobs](const BlockIndex& block, const Facing4&)
	{
		bool result = !blocks.isReserved(block, faction) && hasCrafting.getJobForAtLocation(actorIndex, skillType, block, excludeJobs) != nullptr;
		if(result)
			return std::make_pair(true, block);
		return std::make_pair(false, BlockIndex::null());
	};
	constexpr bool anyOccupied = true;
	return terrainFacade.findPathToConditionBreadthFirst<anyOccupied, decltype(predicate)>(predicate, memo, start, facing, shape, detour, adjacent, faction, maxRange);
}
void CraftPathRequest::writeStep(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(actorIndex, m_craftObjective);
		return;
	}
	if(result.useCurrentPosition)
	{
		if(blocks.isReserved(actors.getLocation(actorIndex), actors.getFaction(actorIndex)))
		{
				actors.objective_canNotCompleteSubobjective(actorIndex);
				return;
		}
	}
	else if(blocks.isReserved(result.blockThatPassedPredicate, actors.getFaction(actorIndex)))
	{
		actors.objective_canNotCompleteSubobjective(actorIndex);
		return;
	}
	BlockIndex block = result.blockThatPassedPredicate;
	SkillTypeId skillType = m_craftObjective.m_skillType;
	FactionId faction = actors.getFactionId(actorIndex);
	auto pair = std::make_pair(area.m_hasCraftingLocationsAndJobs.getForFaction(faction).getJobForAtLocation(actorIndex, skillType, block, m_craftObjective.getFailedJobs()), block);
	m_craftJob = pair.first;
	m_location = pair.second;

	if(m_craftJob  == nullptr)
		actors.objective_canNotCompleteObjective(actorIndex, m_craftObjective);
	else
	{
		// Selected work loctaion has been reserved, try again.
		FactionId faction = actors.getFactionId(actorIndex);
		if(area.getBlocks().isReserved(m_location, faction))
		{
			m_craftObjective.reset(area, actorIndex);
			m_craftObjective.execute(area, actorIndex);
		}
		else
		{
			HasCraftingLocationsAndJobsForFaction& hasCrafting = area.m_hasCraftingLocationsAndJobs.getForFaction(faction);
			hasCrafting.makeAndAssignStepProject(*m_craftJob, m_location, m_craftObjective, actorIndex);
			m_craftObjective.m_craftJob = m_craftJob;
		}
	}
}
Json CraftPathRequest::toJson() const
{
	Json output = PathRequestBreadthFirst::toJson();
	output["objective"] = &m_craftObjective;
	output["job"] = m_craftJob;
	output["location"] = m_location;
	output["type"] = "craft";
	return output;
}
// ObjectiveType.
CraftObjectiveType::CraftObjectiveType(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : m_skillType(data["skillType"].get<SkillTypeId>()) { }
bool CraftObjectiveType::canBeAssigned(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	auto& hasCrafting = area.m_hasCraftingLocationsAndJobs.getForFaction(actors.getFactionId(actor));
	if(!hasCrafting.m_unassignedProjectsBySkill.contains(m_skillType))
	{
		// No jobs needing this skill.
		return false;
	}
	// Check if there are any locations designated for this step category.
	for(CraftJob* craftJob : hasCrafting.m_unassignedProjectsBySkill[m_skillType])
	{
		CraftStepTypeCategoryId category = craftJob->stepIterator->craftStepTypeCategory;
		if(hasCrafting.m_locationsByCategory.contains(category))
			return true;
	}
	return false;
}
std::unique_ptr<Objective> CraftObjectiveType::makeFor(Area&, const ActorIndex&) const
{
	return std::make_unique<CraftObjective>(m_skillType);
}
std::wstring CraftObjectiveType::name() const { return L"craft: " + SkillType::getName(m_skillType); }
// Objective.
CraftObjective::CraftObjective(SkillTypeId st) : Objective(Config::craftObjectivePriority), m_skillType(st) { }
CraftObjective::CraftObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo),
	m_skillType(data["skillType"].get<SkillTypeId>()), m_craftJob(deserializationMemo.m_craftJobs.at(data["craftJob"].get<uintptr_t>()))
{
	if(data.contains("failedJobs"))
		for(const Json& job : data["failedJobs"])
			m_failedJobs.insert(deserializationMemo.m_craftJobs.at(job.get<uintptr_t>()));
}
Json CraftObjective::toJson() const
{
	Json data = Objective::toJson();
	data["skillType"] = m_skillType;
	data["craftJob"] = m_craftJob;
	if(!m_failedJobs.empty())
	{
		data["failedJobs"] = Json::array();
		for(CraftJob* job : m_failedJobs)
			data["failedJobs"].push_back(job);
	}
	return data;
}
std::wstring CraftObjective::name() const { return L"craft: " + SkillType::getName(m_skillType); }
void CraftObjective::execute(Area& area, const ActorIndex& actor)
{
	if(m_craftJob)
		m_craftJob->craftStepProject->commandWorker(actor);
	else
		area.getActors().move_pathRequestRecord(actor, std::make_unique<CraftPathRequest>(area, *this, actor));
}
void CraftObjective::cancel(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
	if(m_craftJob && m_craftJob->craftStepProject)
		m_craftJob->craftStepProject->cancel();
}
void CraftObjective::reset(Area& area, const ActorIndex& actor)
{
	area.getActors().canReserve_clearAll(actor);
	m_craftJob = nullptr;
	cancel(area, actor);
}
