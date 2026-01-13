#include "mistakeAtWork.h"
#include "../../area/area.h"
#include "../../space/space.h"
#include "../../actors/actors.h"
#include "../../items/items.h"
#include "../../simulation/simulation.h"
#include "../../project.h"
#include "../../body.h"
#include "../../hit.h"
#include "../../psycology/psycologyData.h"
#include "../../config/social.h"
#include "../../config/drama.h"
#include "../../objectives/chastise.h"
MistakeAtWorkDramaArc::MistakeAtWorkDramaArc(DramaEngine& engine, Area& area) :
	DramaArc(engine, DramaArcType::MistakeAtWork, &area), m_scheduledEvent(area.m_eventSchedule)
{ schedule(); }
MistakeAtWorkDramaArc::MistakeAtWorkDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine) :
	DramaArc(data, deserializationMemo, dramaEngine),
	m_scheduledEvent(m_area->m_eventSchedule)
{
	m_scheduledEvent.schedule(data["duration"].get<Step>(), *this, m_area->m_simulation, data["start"].get<Step>());
}
Json MistakeAtWorkDramaArc::toJson() const
{
	Json data = DramaArc::toJson();
	data["duration"] = m_scheduledEvent.duration();
	data["start"] = m_scheduledEvent.getStartStep();
	return data;
}
void MistakeAtWorkDramaArc::schedule()
{
	auto& random = m_area->m_simulation.m_random;
	Step duration = Step::create(random.getInRange((5u * Config::stepsPerDay.get()), (15u * Config::stepsPerDay.get())));
	m_scheduledEvent.schedule(duration, *this, m_area->m_simulation);
}
std::pair<ActorReference, std::string> MistakeAtWorkDramaArc::doSwitchMaybeReturnVictim(const MistakeAtWorkType& mistakeType, Project& project, const ActorIndex& perpetrator)
{
	ActorReference victim;
	std::string description;
	Items& items = m_area->getItems();
	Actors& actors = m_area->getActors();
	switch (mistakeType)
	{
		case MistakeAtWorkType::WastedTime:
			project.reset();
			description += "wasted time.";
			break;
		case MistakeAtWorkType::DestroyedInput:
		{
			const ItemIndex& item = project.getRandomItemToConsume();
			if(!item.exists())
			{
				return doSwitchMaybeReturnVictim(MistakeAtWorkType::WastedTime, project, perpetrator);
			}
			if(items.getQuantity(item) > 1)
				items.removeQuantity(item, {1});
			else
				items.destroy(item);
			description += "destruction of a " + ItemType::getName(items.getItemType(item));
			break;
		}
		case MistakeAtWorkType::DamagedUnconsumedItem:
		{
			const ItemIndex& item = project.getRandomUnconsumedItem();
			if(!item.exists())
			{
				return doSwitchMaybeReturnVictim(MistakeAtWorkType::DestroyedInput, project, perpetrator);
			}
			const Percent wear = items.getWear(item);
			if(wear + Config::Drama::percentWearToAddForMistakeAtWork > 100)
			{
				items.destroy(item);
				description += "destruction of a " + ItemType::getName(items.getItemType(item));
			}
			else
			{
				items.setWear(item, wear + Config::Drama::percentWearToAddForMistakeAtWork);
				description += "damage of a " + ItemType::getName(items.getItemType(item));
			}
			break;
		}
		case MistakeAtWorkType::HurtSomeone:
		{
			const auto& actorRefAndProjectWorker = m_area->m_simulation.m_random.getInVector(project.getWorkers().m_data);
			victim = actorRefAndProjectWorker.first;
			const ActorIndex& victimIndex = victim.getIndex(actors.m_referenceData);
			const ItemIndex& tool = project.getRandomUnconsumedItem();
			const Force hitForce = Force::create(actors.getStrength(perpetrator).get() * Config::unitsOfAttackForcePerUnitOfStrength);
			// TODO: Higher skill selects more important body parts to hit.
			BodyPart& bodyPart = actors.body_pickABodyPartByVolume(victimIndex);
			auto& random = m_area->m_simulation.m_random;
			int hitArea = random.getInRange(Config::accidentalHitSmallestArea, Config::accidentalHitLargestArea);
			// TODO: Generate wountType from hit area.
			const WoundType woundType = random.getInEnum<WoundType>();
			Hit hit(hitArea, hitForce, items.getMaterialType(tool), woundType);
			actors.takeHit(victimIndex, hit, bodyPart);
			break;
		}
		default:
			std::unreachable();
	}
	return {victim, description};
}
void MistakeAtWorkDramaArc::callback()
{
	// Choose project
	const SmallSet<Project*> candidateProjects;
	// Query projects with workers.
	auto condition = [&](const Project& project){ return project.inProgress(); };
	Space& space = m_area->getSpace();
	Actors& actors = m_area->getActors();
	// TODO: use different event times for different factions instead of running them all at once.
	for(const auto& [faction, data] : space.project_getAll())
	{
		Project* projectPtr = space.project_randomForFactionWithCondition(faction, condition, *m_area);
		if(projectPtr == nullptr)
			// No suitable projects for faction.
			continue;
		Project& project = *projectPtr;
		std::string description;
		const ActorIndex& perpetrator = m_area->m_simulation.m_random.getInVector(project.getWorkers().m_data).first.getIndex(actors.m_referenceData);
		description += actors.getName(perpetrator) + " made a mistake while working on " + project.description() + " resulting in ";
		MistakeAtWorkType mistakeType = m_area->m_simulation.m_random.getInEnum<MistakeAtWorkType>();
		// Make a copy before maybe reseting the project.
		SmallSet<ActorIndex> coworkers;
		for(const auto& [worker, projectWorker] : project.getWorkers())
			coworkers.insert(worker.getIndex(actors.m_referenceData));
		bool death = false;
		const auto [victim, switchDescription] = doSwitchMaybeReturnVictim(mistakeType, project, perpetrator);
		ActorIndex victimIndex;
		if(victim.exists())
		{
			victimIndex = victim.getIndex(actors.m_referenceData);
			if(!actors.isAlive(victimIndex))
			{
				description += "death of " + actors.getName(victimIndex);
				death = true;
				// TODO: trigger AccidentalHomicideDramaArc.
			}
			else
			{
				description += "injury of " + actors.getName(victimIndex);
				// TODO: trigger AccidentalInjuryDramaArc.
			}
		}
		else
			description += switchDescription;
		// No need to check for chastiser if pepetrator died.
		if(death && victimIndex == perpetrator)
			return;
		// Maybe find someone to chastise perpetrator later.
		ActorIndex chastiser;
		// Start by searching coworkers, if no one suitable is found search canBeSeenBy.
		auto castingCall = [&](const ActorIndex& candidate) -> float
		{
			if(actors.objective_getCurrent<Objective>(candidate).m_priority >= Config::Social::socialPriorityHigh)
				return FLT_MIN;
			if(candidate == perpetrator)
				return FLT_MIN;
			// We are looking for someone who is more skilled, has anger issues, or both.
			const SkillTypeId skillType = project.getSkill();
			const SkillLevel candidateSkillLevel = actors.skill_getLevel(candidate, skillType);
			const SkillLevel perpetratorSkillLevel = actors.skill_getLevel(perpetrator, skillType);
			if(candidateSkillLevel < perpetratorSkillLevel * Config::Social::minimumRatioOfSkillLevelsToChastise)
				return FLT_MIN;
			const PsycologyWeight anger = actors.psycology_get(candidate).getValueFor(PsycologyAttribute::Anger);
			float output = (float)anger.get();
			if(candidateSkillLevel > perpetratorSkillLevel)
				output += candidateSkillLevel.get() - perpetratorSkillLevel.get();
			return output;
		};
		auto castingCallRef = [&](const ActorReference& ref) -> float
		{
			const ActorIndex& candidate = ref.getIndex(actors.m_referenceData);
			return castingCall(candidate);
		};
		const ActorIndex& bestCoworkerCandidate = *std::ranges::max_element(coworkers.m_data, {}, castingCall);
		if(castingCall(bestCoworkerCandidate) >= Config::Social::minimumCastingScoreForChastiseMistakeAtWork)
			chastiser = bestCoworkerCandidate;
		else
		{
			SmallSet<ActorReference> canBeSeenBy = actors.vision_getCanBeSeenBy(perpetrator);
			const ActorReference& bestCanBeSeenByCandidiate = *std::ranges::max_element(canBeSeenBy.m_data, {}, castingCallRef);
			if(castingCallRef(bestCanBeSeenByCandidiate) >= Config::Social::minimumCastingScoreForChastiseMistakeAtWork)
				chastiser = bestCanBeSeenByCandidiate.getIndex(actors.m_referenceData);
		}
		if(chastiser.exists())
		{
			int duration = m_area->m_simulation.m_random.getInRange(Config::Social::minimumDurationToChastiseCycles, Config::Social::maximumDurationToChastiseCycles);
			if(death)
				duration *= Config::Social::multipleForChastiseDurationIfSomeoneWasKilled;
			else if(mistakeType == MistakeAtWorkType::HurtSomeone)
				duration *= Config::Social::multipleForChastiseDurationIfSomeoneWasHurt;
			bool angry = actors.psycology_get(chastiser).getValueFor(PsycologyAttribute::Anger) >= Config::Social::minimumAngerForChastisingToBeAngry;
			const SkillTypeId releventSkill = project.getSkill();
			actors.objective_addNeed(chastiser, std::make_unique<ChastiseObjective>(*m_area, perpetrator, std::move(description), releventSkill, duration, angry));
		}
	}
	schedule();
}