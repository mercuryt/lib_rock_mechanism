
#include "successAtWork.h"
#include "../../area/area.h"
#include "../../items/items.h"
#include "../../space/space.h"
#include "../../actors/actors.h"
#include "../../simulation/simulation.h"
#include "../../project.h"
#include "../../body.h"
#include "../../hit.h"
#include "../../config/social.h"
#include "../../config/drama.h"
#include "../../psycology/relationship.h"
#include "../../objectives/praise.h"
SuccessAtWorkDramaArc::SuccessAtWorkDramaArc(DramaEngine& engine, Area& area) :
	DramaArc(engine, DramaArcType::SuccessAtWork, &area), m_scheduledEvent(area.m_eventSchedule)
{ schedule(); }
SuccessAtWorkDramaArc::SuccessAtWorkDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine) :
	DramaArc(data, deserializationMemo, dramaEngine),
	m_scheduledEvent(m_area->m_eventSchedule)
{
	m_scheduledEvent.schedule(data["duration"].get<Step>(), *this, m_area->m_simulation, data["start"].get<Step>());
}
Json SuccessAtWorkDramaArc::toJson() const
{
	Json data = DramaArc::toJson();
	data["duration"] = m_scheduledEvent.duration();
	data["start"] = m_scheduledEvent.getStartStep();
	return data;
}
void SuccessAtWorkDramaArc::schedule()
{
	auto& random = m_area->m_simulation.m_random;
	Step duration = Step::create(random.getInRange((5u * Config::stepsPerDay.get()), (15u * Config::stepsPerDay.get())));
	m_scheduledEvent.schedule(duration, *this, m_area->m_simulation);
}
std::string SuccessAtWorkDramaArc::doSwitch(const SuccessAtWorkType& successType, Project& project)
{
	std::string description;
	Items& items = m_area->getItems();
	switch (successType)
	{
		case SuccessAtWorkType::SavedTime:
			project.addBonusProduction(Config::Drama::bonusPercentToAddToProjectOnSucessAtWorkSavedTime);
			description += "saved time";
			break;
		case SuccessAtWorkType::SavedInputItems:
		{
			const ItemReference& ref = items.getReference(project.getRandomItemToConsume());
			// No consumable items.
			if(ref.empty())
				doSwitch(SuccessAtWorkType::SavedTime, project);
			else
			{
				project.removeItemFromConsumed(ref);
				const ItemIndex& item = ref.getIndex(items.m_referenceData);
				description += "saved a " + ItemType::getName(items.getItemType(item));
			}
			break;
		}
		case SuccessAtWorkType::HighQuality:
		{
			if(!project.hasQuality())
				doSwitch(SuccessAtWorkType::SavedInputItems, project);
			else
			{
				project.m_qualityBonus = true;
				description += "high quality";
			}
			break;
		}
		default:
			std::unreachable();
	}
	return description;
}
void SuccessAtWorkDramaArc::callback()
{
	// Choose project
	const SmallSet<Project*> candidateProjects;
	// Query projects with workers.
	auto condition = [&](const RTreeDataWrapper<Project*, nullptr>& project){ return project.get()->inProgress(); };
	Actors& actors = m_area->getActors();
	Space& space = m_area->getSpace();
	// TODO: use different event times for different factions instead of running them all at once.
	for(auto& [faction, data] : space.project_getAll())
	{
		SmallSet<RTreeDataWrapper<Project*, nullptr>> projects = data.getAllWithCondition(condition);
		if(projects.empty())
			continue;
		std::string description;
		Project& project = *m_area->m_simulation.m_random.getInVector(projects.m_data).get();
		const ActorIndex& worker = m_area->m_simulation.m_random.getInVector(project.getWorkers().m_data).first.getIndex(actors.m_referenceData);
		description += actors.getName(worker) + " made a success while working on " + project.description() + " resulting in ";
		SuccessAtWorkType successType = m_area->m_simulation.m_random.getInEnum<SuccessAtWorkType>();
		// Make a copy before maybe completing the project.
		SmallSet<ActorIndex> coworkers;
		const auto& projectWorkers = project.getWorkers();
		coworkers.reserve(projectWorkers.size());
		for(const auto& pair : projectWorkers)
			coworkers.insert(pair.first.getIndex(actors.m_referenceData));
		description += doSwitch(successType, project);
		const SkillTypeId skillType = project.getSkill();
		if(skillType.exists())
			actors.skill_addXp(worker, skillType, Config::skillPointsToAddWhenSuccessAtWork);
		// Maybe find someone to praise worker later.
		ActorIndex praiser;
		// Start by searching coworkers, if no one suitable is found search canBeSeenBy.
		auto castingCall = [&](const ActorIndex& index) -> float
		{
			if(actors.objective_getCurrent<Objective>(index).m_priority >= Config::Social::socialPriorityHigh)
				return FLT_MIN;
			if(index == worker)
				return FLT_MIN;
			// We are looking for someone who has a positive relationship.
			const RelationshipBetweenActors* relationship = actors.psycology_get(index).getRelationships().maybeGetForActor(actors.getId(worker));
			if(relationship == nullptr)
				return 0;
			return relationship->positivity.get();
		};
		auto castingCallRef = [&](const ActorReference& ref) -> float
		{
			const ActorIndex& candidate = ref.getIndex(actors.m_referenceData);
			return castingCall(candidate);
		};
		const ActorIndex& bestCoworkerCandidate = *std::ranges::max_element(coworkers.m_data, {}, castingCall);
		if(castingCall(bestCoworkerCandidate) >= Config::Social::minimumCastingScoreForPraiseSuccessAtWork)
			praiser = bestCoworkerCandidate;
		else
		{
			SmallSet<ActorReference> canBeSeenBy = actors.vision_getCanBeSeenBy(worker);
			const ActorReference& bestCanBeSeenByCandidiate = *std::ranges::max_element(canBeSeenBy.m_data, {}, castingCallRef);
			if(castingCallRef(bestCanBeSeenByCandidiate) >= Config::Social::minimumCastingScoreForPraiseSuccessAtWork)
				praiser = bestCanBeSeenByCandidiate.getIndex(actors.m_referenceData);
		}
		if(praiser.exists())
			actors.objective_addNeed(praiser, std::make_unique<PraiseObjective>(*m_area, worker, description));
	}
	schedule();
}