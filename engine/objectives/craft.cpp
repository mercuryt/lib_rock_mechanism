#include "craft.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../area.h"
CraftPathRequest::CraftPathRequest(Area& area, CraftObjective& co, ActorIndex actor) : m_craftObjective(co)
{
	assert(m_craftObjective.m_craftJob == nullptr);
	Actors& actors = area.getActors();
	FactionId faction = actors.getFactionId(actor);
	HasCraftingLocationsAndJobsForFaction& hasCrafting = area.m_hasCraftingLocationsAndJobs.getForFaction(faction);
	Blocks& blocks = area.getBlocks();
	SkillTypeId skillType = m_craftObjective.m_skillType;
	auto& excludeJobs = m_craftObjective.getFailedJobs();
	std::function<bool(BlockIndex, Facing)> predicate = [&blocks, faction, &hasCrafting, actor, skillType, &excludeJobs](BlockIndex block, Facing) mutable -> bool 
	{
		return !blocks.isReserved(block, faction) && hasCrafting.getJobForAtLocation(actor, skillType, block, excludeJobs) != nullptr;
	};
	bool unreserved = true;
	createGoToCondition(area, actor, predicate, m_craftObjective.m_detour, unreserved, Config::maxRangeToSearchForCraftingRequirements, BlockIndex::null());
}
CraftPathRequest::CraftPathRequest(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_craftObjective(static_cast<CraftObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))),
	m_craftJob(deserializationMemo.m_craftJobs.at(data["craftJob"].get<uintptr_t>())),
	m_location(data["location"].get<BlockIndex>())
{
	nlohmann::from_json(data, *this);
}
void CraftPathRequest::callback(Area& area, FindPathResult& result)
{
	Actors& actors = area.getActors();
	ActorIndex actor = getActor();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(actor, m_craftObjective);
		return;
	}
	if(result.useCurrentPosition)
	{
		if(!actors.move_tryToReserveOccupied(actor))
		{
			actors.objective_canNotCompleteSubobjective(actor);
			return;
		}
	}
	else if(!actors.move_tryToReserveProposedDestination(actor, result.path))
	{
		actors.objective_canNotCompleteSubobjective(actor);
		return;
	}
	BlockIndex block = result.blockThatPassedPredicate;
	SkillTypeId skillType = m_craftObjective.m_skillType;
	FactionId faction = actors.getFactionId(actor);
	auto pair = std::make_pair(area.m_hasCraftingLocationsAndJobs.getForFaction(faction).getJobForAtLocation(actor, skillType, block, m_craftObjective.getFailedJobs()), block);
	m_craftJob = pair.first;
	m_location = pair.second;

	if(m_craftJob  == nullptr)
		actors.objective_canNotCompleteObjective(actor, m_craftObjective);
	else
	{
		// Selected work loctaion has been reserved, try again.
		FactionId faction = actors.getFactionId(actor);
		if(area.getBlocks().isReserved(m_location, faction))
		{
			m_craftObjective.reset(area, actor);
			m_craftObjective.execute(area, actor);
		}
		else
		{
			HasCraftingLocationsAndJobsForFaction& hasCrafting = area.m_hasCraftingLocationsAndJobs.getForFaction(faction);
			hasCrafting.makeAndAssignStepProject(*m_craftJob, m_location, m_craftObjective, actor);
			m_craftObjective.m_craftJob = m_craftJob;
		}
	}
}
Json CraftPathRequest::toJson() const
{
	Json output;
	nlohmann::to_json(output, *this);
	output["objective"] = &m_craftObjective;
	output["job"] = m_craftJob;
	output["location"] = m_location;
	return output;
}
// ObjectiveType.
CraftObjectiveType::CraftObjectiveType(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : m_skillType(data["skillType"].get<SkillTypeId>()) { }
bool CraftObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	Actors& actors = area.getActors();
	auto& hasCrafting = area.m_hasCraftingLocationsAndJobs.getForFaction(actors.getFactionId(actor));
	if(!hasCrafting.m_unassignedProjectsBySkill.contains(m_skillType))
	{
		// No jobs needing this skill.
		return false;
	}
	// Check if there are any locations designated for this step category.
	for(CraftJob* craftJob : hasCrafting.m_unassignedProjectsBySkill.at(m_skillType))
	{
		CraftStepTypeCategoryId category = craftJob->stepIterator->craftStepTypeCategory;
		if(hasCrafting.m_locationsByCategory.contains(category))
			return true;
	}
	return false;
}
std::unique_ptr<Objective> CraftObjectiveType::makeFor(Area&, ActorIndex) const
{
	return std::make_unique<CraftObjective>(m_skillType);
}
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
void CraftObjective::execute(Area& area, ActorIndex actor)
{
	if(m_craftJob)
		m_craftJob->craftStepProject->commandWorker(actor);
	else
		area.getActors().move_pathRequestRecord(actor, std::make_unique<CraftPathRequest>(area, *this, actor));
}
void CraftObjective::cancel(Area& area, ActorIndex actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
	if(m_craftJob && m_craftJob->craftStepProject)
		m_craftJob->craftStepProject->cancel();
}
void CraftObjective::reset(Area& area, ActorIndex actor)
{
	area.getActors().canReserve_clearAll(actor);
	m_craftJob = nullptr;
	cancel(area, actor);
}
