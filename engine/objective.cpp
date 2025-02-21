//ObjectiveTypePrioritySet
#include "objective.h"
#include "actorOrItemIndex.h"
#include "area/area.h"
#include "deserializationMemo.h"
#include "simulation/simulation.h"
#include "simulation/hasActors.h"
#include "actors/actors.h"

#include "objectives/givePlantsFluid.h"
#include "objectives/harvest.h"
#include "objectives/rest.h"
#include "objectives/sowSeeds.h"
#include "objectives/wander.h"
#include "objectives/woodcutting.h"
#include "objectives/craft.h"
#include "objectives/dig.h"
#include "objectives/construct.h"
#include "objectives/stockpile.h"
#include "objectives/rest.h"
#include "objectives/sleep.h"
#include "objectives/eat.h"
#include "objectives/drink.h"
#include "objectives/getToSafeTemperature.h"
#include "types.h"
#include "portables.hpp"

#include <cstdio>
#include <numbers>

// Input.
/*
void ObjectiveTypeSetPriorityInputAction::execute()
{
	m_actor.m_hasObjectives.m_prioritySet.setPriority(m_objectiveType, m_priority);
}
void ObjectiveTypeRemoveInputAction::execute()
{
	m_actor.m_hasObjectives.m_prioritySet.remove(m_objectiveType);
}
*/
void ObjectiveTypePrioritySet::load(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo)
{
	m_data = data["data"].get<std::vector<ObjectivePriority>>();
}
Json ObjectiveTypePrioritySet::toJson() const
{
	return Json{{"data", m_data}};
}
ObjectivePriority& ObjectiveTypePrioritySet::getById(const ObjectiveTypeId& objectiveTypeId)
{
	auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == objectiveTypeId; });
	assert(found != m_data.end());
	return *found;
}
const ObjectivePriority& ObjectiveTypePrioritySet::getById(const ObjectiveTypeId& objectiveTypeId) const
{
	return const_cast<ObjectiveTypePrioritySet*>(this)->getById(objectiveTypeId);
}
void ObjectiveTypePrioritySet::setPriority(Area& area, const ActorIndex& actor, const ObjectiveTypeId& objectiveTypeId, const Priority& priority)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& x) { return x.objectiveType == objectiveTypeId; });
	if(found == m_data.end())
		m_data.emplace_back(objectiveTypeId, priority, Step::create(0)); // waitUntillStepBeforeAssigningAgain initilizes to 0.
	else
		found->priority = priority;
	std::ranges::sort(m_data, std::ranges::greater{}, &ObjectivePriority::priority);
	area.getActors().objective_maybeDoNext(actor);
}
void ObjectiveTypePrioritySet::remove(const ObjectiveTypeId& objectiveType)
{
	std::erase_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Area& area, const ActorIndex& actor)
{
	assert(!area.getActors().objective_exists(actor));
	Step currentStep = area.m_simulation.m_step;
	for(auto& objectivePriority : m_data)
	{
		const ObjectiveType& objectiveType = ObjectiveType::getById(objectivePriority.objectiveType);
		if(currentStep > objectivePriority.doNotAssignAgainUntil && objectiveType.canBeAssigned(area, actor))
		{
			area.getActors().objective_addTaskToStart(actor, objectiveType.makeFor(area, actor));
			return;
		}
	}
	// No assignable tasks, do an idle task.
	if(!area.getActors().stamina_isFull(actor))
		area.getActors().objective_addTaskToStart(actor, std::make_unique<RestObjective>(area));
	else
		area.getActors().objective_addTaskToStart(actor, std::make_unique<WanderObjective>());
}
void ObjectiveTypePrioritySet::setDelay(Area& area, const ObjectiveTypeId& objectiveTypeId)
{
	auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == objectiveTypeId; });
	// If found is not in data it was assigned by some other means (such as player interaction) so we don't need a delay.
	if(found != m_data.end())
		found->doNotAssignAgainUntil = area.m_simulation.m_step + Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective;
}
Priority ObjectiveTypePrioritySet::getPriorityFor(const ObjectiveTypeId& objectiveTypeId) const
{
	const auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == objectiveTypeId; });
	if(found == m_data.end())
		return Priority::create(0);
	return found->priority;
}
bool ObjectiveTypePrioritySet::isOnDelay(Area& area, const ObjectiveTypeId& objectiveTypeId) const
{
	const ObjectivePriority& objectivePriority = getById(objectiveTypeId);
	return objectivePriority.doNotAssignAgainUntil > area.m_simulation.m_step;
}

Step ObjectiveTypePrioritySet::getDelayEndFor(const ObjectiveTypeId& objectiveTypeId) const
{
	const ObjectivePriority& objectivePriority = getById(objectiveTypeId);
	return objectivePriority.doNotAssignAgainUntil;
}
// SupressedNeed
SupressedNeed::SupressedNeed(Area& area, std::unique_ptr<Objective> o, const ActorReference& ref) :
	m_objective(std::move(o)), m_event(area.m_eventSchedule), m_actor(ref)
{
	m_event.schedule(area, *this);
}
SupressedNeed::SupressedNeed(Area& area, const Json& data, DeserializationMemo& deserializationMemo, const ActorReference& ref) :
	m_event(area.m_eventSchedule), m_actor(ref)
{
	m_objective = deserializationMemo.loadObjective(data["objective"], area, m_actor.getIndex(area.getActors().m_referenceData));
	if(data.contains("eventStart"))
		m_event.schedule(area, *this, data["eventStart"].get<Step>());
}
Json SupressedNeed::toJson() const
{
	 Json data{{"objective", m_objective->toJson()}};
	 if(m_event.exists())
		 data["eventStart"] = m_event.getStartStep();
	 return data;
}
void SupressedNeed::callback(Area& area)
{
	ActorIndex actor = m_actor.getIndex(area.getActors().m_referenceData);
	auto objective = std::move(m_objective);
	HasObjectives& hasObjectives = *area.getActors().m_hasObjectives[actor];
	hasObjectives.m_supressedNeeds.erase(objective->getNeedType());
	hasObjectives.addNeed(area, std::move(objective));
}
SupressedNeedEvent::SupressedNeedEvent(Area& area, SupressedNeed& sn, const Step start) :
	ScheduledEvent(area.m_simulation, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective, start), m_supressedNeed(sn) { }
void SupressedNeedEvent::execute(Simulation&, Area* area) { m_supressedNeed.callback(*area); }
void SupressedNeedEvent::clearReferences(Simulation&, Area*) { m_supressedNeed.m_event.clearPointer(); }
// Static method.
void ObjectiveType::load()
{
	objectiveTypeData.resize(16);
	ObjectiveTypeId index = ObjectiveTypeId::create(0);
	objectiveTypeData[index++] = std::make_unique<CraftObjectiveType>(SkillType::byName(L"wood working"));
	objectiveTypeData[index++] = std::make_unique<CraftObjectiveType>(SkillType::byName(L"metal working"));
	objectiveTypeData[index++] = std::make_unique<CraftObjectiveType>(SkillType::byName(L"stone carving"));
	objectiveTypeData[index++] = std::make_unique<CraftObjectiveType>(SkillType::byName(L"bone carving"));
	objectiveTypeData[index++] = std::make_unique<CraftObjectiveType>(SkillType::byName(L"assembling"));
	objectiveTypeData[index++] = std::make_unique<DigObjectiveType>();
	//  TODO: specalize construct into Earthworks, Masonry, Metal Construction, and Carpentry.
	objectiveTypeData[index++] = std::make_unique<ConstructObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<StockPileObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<SowSeedsObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<GivePlantsFluidObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<HarvestObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<WoodCuttingObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<SleepObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<EatObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<DrinkObjectiveType>();
	objectiveTypeData[index++] = std::make_unique<GetToSafeTemperatureObjectiveType>();
}
// Static method.
ObjectiveTypeId ObjectiveType::getIdByName(std::wstring name)
{
	const ObjectiveType& objectiveType = getByName(name);
	return objectiveType.getId();
}
// Static method.
const ObjectiveType& ObjectiveType::getByName(std::wstring name)
{
	auto found = std::ranges::find_if(objectiveTypeData, [&name](const auto& type) { return type->name() == name; });
	assert(found != objectiveTypeData.end());
	return *found->get();
}
// Static method.
const ObjectiveType& ObjectiveType::getById(const ObjectiveTypeId& id) { return *objectiveTypeData[id].get(); }
ObjectiveTypeId ObjectiveType::getId() const
{
	auto iter = objectiveTypeData.find_if([&](const std::unique_ptr<ObjectiveType>& type) { return type.get() == this; });
	assert(iter != objectiveTypeData.end());
	uint distance = std::distance(objectiveTypeData.begin(), iter);
	return ObjectiveTypeId::create(distance);
}
// Objective.
Objective::Objective(const Priority& priority) : m_priority(priority) { }
Objective::Objective(const Json& data, DeserializationMemo& deserializationMemo) :
	m_priority(data["priority"].get<Priority>()),
	m_detour(data["detour"].get<bool>())
{
	uintptr_t address = data["address"].get<uintptr_t>();
	assert(!deserializationMemo.m_objectives.contains(address));
	deserializationMemo.m_objectives[address] = this;
}
Json Objective::toJson() const
{
	return Json{{"priority", m_priority}, {"detour", m_detour}, {"address", reinterpret_cast<uintptr_t>(this)}, {"type", name()}};
}
CannotCompleteObjectiveDishonorCallback::CannotCompleteObjectiveDishonorCallback(Area& area, const Json& data) :
	m_area(area),
	m_actor(data["actor"], area.getActors().m_referenceData) { }
Json CannotCompleteObjectiveDishonorCallback::toJson() const
{
	return {{"actor", m_actor}};
}
void CannotCompleteObjectiveDishonorCallback::execute(const Quantity&, const Quantity&)
{
	Actors& actors = m_area.getActors();
	actors.objective_canNotCompleteSubobjective(m_actor.getIndex(actors.m_referenceData)); }
// HasObjectives.
void HasObjectives::load(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const ActorIndex& actor)
{
	for(const Json& objective : data["needsQueue"])
	{
		m_needsQueue.push_back(deserializationMemo.loadObjective(objective, area, actor));
		m_typesOfNeedsInQueue.insert(m_needsQueue.back()->getNeedType());
	}
	for(const Json& objective : data["tasksQueue"])
		m_tasksQueue.push_back(deserializationMemo.loadObjective(objective, area, actor));
	if(data.contains("currentObjective"))
		m_currentObjective = deserializationMemo.m_objectives.at(data["currentObjective"].get<uintptr_t>());
	for(const Json& pair : data["supressedNeeds"])
		m_supressedNeeds.emplace(pair[0].get<NeedType>(), area, pair[1], deserializationMemo, area.getActors().getReference(m_actor));
	m_prioritySet.load(data["prioritySet"], deserializationMemo);
}
Json HasObjectives::toJson() const
{
	Json data;
	data["needsQueue"] = Json::array();
	for(auto& objective : m_needsQueue)
		data["needsQueue"].push_back(objective->toJson());
	data["tasksQueue"] = Json::array();
	for(auto& objective : m_tasksQueue)
		data["tasksQueue"].push_back(objective->toJson());
	if(m_currentObjective)
		data["currentObjective"] = reinterpret_cast<uintptr_t>(m_currentObjective);
	data["supressedNeeds"] = Json::array();
	for(auto& supressedNeed : m_supressedNeeds)
	{
		Json pair{supressedNeed.first, supressedNeed.second->toJson()};
		data["supressedNeeds"].push_back(pair);
	}
	data["prioritySet"] = m_prioritySet.toJson();
	return data;
}
Step HasObjectives::getNeedDelayRemaining(NeedType needType) const
{
	assert(m_supressedNeeds.contains(needType));
	const SupressedNeed& supressedNeed = m_supressedNeeds[needType];
	return supressedNeed.getDelayRemaining();
}
void HasObjectives::getNext(Area& area)
{
	m_currentObjective = nullptr;
	if(m_needsQueue.empty())
	{
		if(m_tasksQueue.empty())
			m_prioritySet.setObjectiveFor(area, m_actor);
		else
			setCurrentObjective(area, *m_tasksQueue.front().get());
	}
	else
	{
		if(m_tasksQueue.empty())
			setCurrentObjective(area, *m_needsQueue.front().get());
		else
		{
			if(m_tasksQueue.front()->m_priority > m_needsQueue.front()->m_priority)
				setCurrentObjective(area, *m_tasksQueue.front().get());
			else
				setCurrentObjective(area, *m_needsQueue.front().get());
		}
	}
}
void HasObjectives::maybeUsurpsPriority(Area& area, Objective& objective)
{
	if(m_currentObjective == nullptr)
		setCurrentObjective(area, objective);
	else
	{
		if(m_currentObjective->m_priority < objective.m_priority)
		{
			if(m_currentObjective->canResume())
				m_currentObjective->delay(area, m_actor);
			else
				m_currentObjective->cancel(area, m_actor);
			setCurrentObjective(area, objective);
		}
	}
}
void HasObjectives::setCurrentObjective(Area& area, Objective& objective)
{
	area.getActors().move_clearPath(m_actor);
	area.getActors().project_maybeUnset(m_actor);
	m_currentObjective = &objective;
	objective.execute(area, m_actor);
}
void HasObjectives::addNeed(Area& area, std::unique_ptr<Objective> objective)
{
	NeedType needType = objective->getNeedType();
	if(hasSupressedNeed(needType))
		return;
	// Wake up to fulfil need.
	if(!area.getActors().sleep_isAwake(m_actor))
		area.getActors().sleep_wakeUpEarly(m_actor);
	Objective* o = objective.get();
	m_needsQueue.push_back(std::move(objective));
	m_needsQueue.sort([](const std::unique_ptr<Objective>& a, const std::unique_ptr<Objective>& b){ return a->m_priority > b->m_priority; });
	m_typesOfNeedsInQueue.insert(needType);
	maybeUsurpsPriority(area, *o);
}
void HasObjectives::addTaskToEnd(Area& area, std::unique_ptr<Objective> objective)
{
	Objective* o = objective.get();
	m_tasksQueue.push_back(std::move(objective));
	if(m_currentObjective == nullptr)
		setCurrentObjective(area, *o);
}
void HasObjectives::addTaskToStart(Area& area, std::unique_ptr<Objective> objective)
{
	Objective* o = objective.get();
	m_tasksQueue.push_front(std::move(objective));
	maybeUsurpsPriority(area, *o);
}
void HasObjectives::replaceTasks(Area& area, std::unique_ptr<Objective> objective)
{
	m_currentObjective = nullptr;
	m_tasksQueue.clear();
	addTaskToStart(area, std::move(objective));
}
// Called when an objective is canceled by the player, canceled by the actor, or completed sucessfully.
// TODO: seperate functions for tasks and needs?
void HasObjectives::destroy(Area& area, Objective& objective)
{
	Actors& actors = area.getActors();
	bool isCurrent = m_currentObjective == &objective;
	if(objective.isNeed() && m_typesOfNeedsInQueue.contains(objective.getNeedType()))
	{
		NeedType needType = objective.getNeedType();
		// Remove canceled objective from needs queue.
		auto found = std::ranges::find(m_needsQueue, needType, [](const auto& o){ return o->getNeedType(); });
		assert(found != m_needsQueue.end());
		m_needsQueue.erase(found);
		m_typesOfNeedsInQueue.erase(needType);
	}
	else
	{
		// Remove canceled objective from task queue.
		assert(m_tasksQueue.end() != std::ranges::find(m_tasksQueue, &objective, [](const auto& o){ return o.get(); }));
		std::erase_if(m_tasksQueue, [&](const auto& o){ return &objective == o.get(); });
	}
	actors.canReserve_clearAll(m_actor);
	actors.reservable_unreserveAll(m_actor);
	actors.maybeLeadAndFollowDisband(m_actor);
	if(actors.canPickUp_exists(m_actor))
	{
		ActorOrItemIndex wasCarrying = area.getActors().canPickUp_tryToPutDownPolymorphic(m_actor, actors.getLocation(m_actor));
		if(wasCarrying.empty())
		{
			// Could not find a place to put down carrying.
			// Wander somewhere else, hopefully we can put down there.
			// This method will be called again when wander finishes.
			actors.objective_addTaskToStart(m_actor, std::make_unique<WanderObjective>());
		}
		if(actors.getFaction(m_actor).exists())
		{
			if(wasCarrying.isItem())
			{
				ItemIndex item = wasCarrying.getItem();
				const FactionId faction = actors.getFaction(m_actor);
				if(area.m_hasStockPiles.contains(faction))
					area.m_hasStockPiles.getForFaction(faction).addItem(item);
			}
			else
			{
				assert(wasCarrying.isActor());
				//TODO: add to medical listing.
			}
		}
	}
	if(isCurrent)
		getNext(area);
}
void HasObjectives::cancel(Area& area, Objective& objective)
{
	objective.cancel(area, m_actor);
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.project_maybeUnset(m_actor);
	destroy(area, objective);
}
void HasObjectives::objectiveComplete(Area& area, Objective& objective)
{
	Actors& actors = area.getActors();
	assert(actors.sleep_isAwake(m_actor));
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.project_maybeUnset(m_actor);
	destroy(area, objective);
}
void HasObjectives::subobjectiveComplete(Area& area)
{
	#ifndef NDEBUG
		Actors& actors = area.getActors();
		assert(actors.sleep_isAwake(m_actor));
	#endif
	if(m_currentObjective == nullptr)
		getNext(area);
	else
		m_currentObjective->execute(area, m_actor);
}
void HasObjectives::cannotCompleteSubobjective(Area& area)
{
	Actors& actors = area.getActors();
	actors.move_clearPath(m_actor);
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.canReserve_clearAll(m_actor);
	//TODO: generate cancelaton message?
	//TODO: mandate existance of objective?
	if(m_currentObjective == nullptr)
		getNext(area);
	else
		//TODO: call reset here?
		m_currentObjective->execute(area, m_actor);
}
Objective& HasObjectives::getCurrent()
{
	assert(m_currentObjective != nullptr);
       	return *m_currentObjective;
}
bool HasObjectives::queuesAreEmpty() const
{
	return m_tasksQueue.empty() && m_needsQueue.empty();
}
bool HasObjectives::hasTask(const ObjectiveTypeId& objectiveTypeId) const
{
	return std::ranges::find(m_tasksQueue, objectiveTypeId, [](const auto& objective){ return objective->getTypeId(); }) != m_tasksQueue.end();
}
bool HasObjectives::hasNeed(const NeedType& needType) const
{
	return std::ranges::find(m_needsQueue, needType, [](const auto& objective){ return objective->getNeedType(); }) != m_needsQueue.end();
}
// Does not use ::cancel because needs to move supressed objective into storage.
void HasObjectives::cannotFulfillNeed(Area& area, Objective& objective)
{
	Actors& actors = area.getActors();
	actors.move_clearPath(m_actor);
	actors.canReserve_clearAll(m_actor);
	NeedType needType = objective.getNeedType();
	assert(!m_supressedNeeds.contains(needType));
	bool isCurrent = m_currentObjective == &objective;
	objective.cancel(area, m_actor);
	auto found = std::ranges::find(m_needsQueue, needType, [](std::unique_ptr<Objective>& o) { return o->getNeedType(); });
	assert(found != m_needsQueue.end());
	// Store supressed need.
	m_supressedNeeds.emplace(needType, area, std::move(*found), area.getActors().getReference(m_actor));
	// Remove from needs queue.
	m_typesOfNeedsInQueue.erase(needType);
	m_needsQueue.erase(found);
	// No need to disband leadAndFollow here, needs don't use it.
	actors.move_pathRequestMaybeCancel(m_actor);
	if(isCurrent)
		getNext(area);
}
void HasObjectives::cannotFulfillObjective(Area& area, Objective& objective)
{
	Actors& actors = area.getActors();
	actors.move_clearPath(m_actor);
	actors.canReserve_clearAll(m_actor);
	actors.maybeLeadAndFollowDisband(m_actor);
	// Store delay to wait before trying again.
	m_prioritySet.setDelay(area, objective.getTypeId());
	cancel(area, objective);
	//TODO: generate cancelation message?
}
void HasObjectives::detour(Area& area)
{
	//TODO: This conditional is required because objectives are not mandated to always exist.
	if(m_currentObjective != nullptr)
		m_currentObjective->detour(area, m_actor);
}
