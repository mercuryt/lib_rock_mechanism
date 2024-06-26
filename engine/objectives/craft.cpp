#include "craft.h"
#include "../area.h"
CraftPathRequest::CraftPathRequest(Area& area, CraftObjective& co) : m_craftObjective(co)
{
	assert(m_craftObjective.m_craftJob == nullptr);
	Actors& actors = area.getActors();
	Faction& faction = *actors.getFaction(m_craftObjective.m_actor);
	HasCraftingLocationsAndJobsForFaction& hasCrafting = area.m_hasCraftingLocationsAndJobs.at(faction);
	Blocks& blocks = area.getBlocks();
	const SkillType& skillType = m_craftObjective.m_skillType;
	auto& excludeJobs = m_craftObjective.getFailedJobs();
	ActorIndex actor = m_craftObjective.m_actor;
	std::function<bool(BlockIndex, Facing)> predicate = [&blocks, &faction, &hasCrafting, actor, &skillType, &excludeJobs](BlockIndex block, Facing) mutable
	{
		return !blocks.isReserved(block, faction) && hasCrafting.getJobForAtLocation(actor, skillType, block, excludeJobs) != nullptr;
	};
	bool unreserved = true;
	createGoToCondition(area, actor, predicate, m_craftObjective.m_detour, unreserved, Config::maxRangeToSearchForCraftingRequirements, BLOCK_INDEX_MAX);
}
void CraftPathRequest::callback(Area& area, FindPathResult result)
{
	Actors& actors = area.getActors();
	if(result.path.empty() && !result.useCurrentPosition)
	{
		actors.objective_canNotCompleteObjective(m_craftObjective.m_actor, m_craftObjective);
		return;
	}
	if(result.useCurrentPosition)
	{
		if(!actors.move_tryToReserveOccupied(m_craftObjective.m_actor))
		{
			actors.objective_canNotCompleteSubobjective(m_craftObjective.m_actor);
			return;
		}
	}
	else if(!actors.move_tryToReserveProposedDestination(m_craftObjective.m_actor, result.path))
	{
		actors.objective_canNotCompleteSubobjective(m_craftObjective.m_actor);
		return;
	}
	BlockIndex block = result.blockThatPassedPredicate;
	const SkillType& skillType = m_craftObjective.m_skillType;
	Faction& faction = *actors.getFaction(m_craftObjective.m_actor);
	auto pair =  std::make_pair(area.m_hasCraftingLocationsAndJobs.at(faction).getJobForAtLocation(m_craftObjective.m_actor, skillType, block, m_craftObjective.getFailedJobs()), block);
	m_craftJob = pair.first;
	m_location = pair.second;

	if(m_craftJob  == nullptr)
		actors.objective_canNotCompleteObjective(m_craftObjective.m_actor, m_craftObjective);
	else
	{
		// Selected work loctaion has been reserved, try again.
		Faction& faction = *actors.getFaction(m_craftObjective.m_actor);
		if(area.getBlocks().isReserved(m_location, faction))
		{
			m_craftObjective.reset(area);
			m_craftObjective.execute(area);
		}
		else
		{
			HasCraftingLocationsAndJobsForFaction& hasCrafting = area.m_hasCraftingLocationsAndJobs.at(faction);
			hasCrafting.makeAndAssignStepProject(*m_craftJob, m_location, m_craftObjective);
			m_craftObjective.m_craftJob = m_craftJob;
		}
	}
}
// ObjectiveType.
CraftObjectiveType::CraftObjectiveType(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : m_skillType(*data["skillType"].get<const SkillType*>()) { }
bool CraftObjectiveType::canBeAssigned(Area& area, ActorIndex actor) const
{
	Actors& actors = area.getActors();
	auto& hasCrafting = area.m_hasCraftingLocationsAndJobs.at(*actors.getFaction(actor));
	if(!hasCrafting.m_unassignedProjectsBySkill.contains(&m_skillType))
	{
		// No jobs needing this skill.
		return false;
	}
	// Check if there are any locations designated for this step category.
	for(CraftJob* craftJob : hasCrafting.m_unassignedProjectsBySkill.at(&m_skillType))
	{
		const CraftStepTypeCategory& category = craftJob->stepIterator->craftStepTypeCategory;
		if(hasCrafting.m_locationsByCategory.contains(&category))
			return true;
	}
	return false;
}
std::unique_ptr<Objective> CraftObjectiveType::makeFor(Area&, ActorIndex actor) const
{
	return std::make_unique<CraftObjective>(actor, m_skillType);
}
Json CraftObjectiveType::toJson() const 
{
	Json data = ObjectiveType::toJson();
	data["skillType"] = m_skillType;
	return data;
}
// Objective.
CraftObjective::CraftObjective(ActorIndex a, const SkillType& st) :
	Objective(a, Config::craftObjectivePriority), m_skillType(st) { }
/*
CraftObjective::CraftObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_skillType(*data["skillType"].get<const SkillType*>()), m_craftJob(deserializationMemo.m_craftJobs.at(data["craftJob"].get<uintptr_t>())), 
	m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine) 
{ 
	if(data.contains("failedJobs"))
		for(const Json& job : data["failedJobs"])
			m_failedJobs.insert(deserializationMemo.m_craftJobs.at(job.get<uintptr_t>()));
	if(data.contains("threadedTask"))
		m_threadedTask.create(*this);
}
Json CraftObjective::toJson() const 
{
	Json data = Objective::toJson();
	data["skillType"] = m_skillType;
	data["craftJob"] = m_craftJob;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	if(!m_failedJobs.empty())
	{
		data["failedJobs"] = Json::array();
		for(CraftJob* job : m_failedJobs)
			data["failedJobs"].push_back(job);
	}
	return data;
}
*/
void CraftObjective::execute(Area& area)
{
	if(m_craftJob)
		m_craftJob->craftStepProject->commandWorker(m_actor);
	else
		area.getActors().move_pathRequestRecord(m_actor, std::make_unique<CraftPathRequest>(area, *this));
}
void CraftObjective::cancel(Area& area)
{
	area.getActors().move_pathRequestMaybeCancel(m_actor);
	if(m_craftJob && m_craftJob->craftStepProject)
		m_craftJob->craftStepProject->cancel();
}
void CraftObjective::reset(Area& area)
{
	area.getActors().canReserve_clearAll(m_actor);
	m_craftJob = nullptr;
	cancel(area);
}
