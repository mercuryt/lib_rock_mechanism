//ObjectiveTypePrioritySet
#include "objective.h"
#include "actorOrItemIndex.h"
#include "area.h"
#include "deserializationMemo.h"
#include "simulation.h"
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
ObjectivePriority& ObjectiveTypePrioritySet::getById(ObjectiveTypeId objectiveTypeId)
{
	auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType->getObjectiveTypeId() == objectiveTypeId; });
	assert(found != m_data.end());
	return *found;
}
const ObjectivePriority& ObjectiveTypePrioritySet::getById(ObjectiveTypeId objectiveTypeId) const
{
	return const_cast<ObjectiveTypePrioritySet*>(this)->getById(objectiveTypeId);
}
void ObjectiveTypePrioritySet::setPriority(Area& area, ActorIndex actor, const ObjectiveType& objectiveType, uint8_t priority)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& x) { return x.objectiveType == &objectiveType; });
	if(found == m_data.end())
		m_data.emplace_back(&objectiveType, priority, 0); // waitUntillStepBeforeAssigningAgain initilizes to 0.
	else
		found->priority = priority;
	std::ranges::sort(m_data, std::ranges::greater{}, &ObjectivePriority::priority);
	area.getActors().objective_maybeDoNext(actor);
}
void ObjectiveTypePrioritySet::remove(const ObjectiveType& objectiveType)
{
	std::erase_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == &objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Area& area, ActorIndex actor)
{
	assert(!area.getActors().objective_exists(actor));
	assert(!area.getActors().objective_queuesAreEmpty(actor));
	Step currentStep = area.m_simulation.m_step;
	for(auto& objectivePriority : m_data)
		if(currentStep > objectivePriority.doNotAssignAgainUntil && objectivePriority.objectiveType->canBeAssigned(area, actor))
		{
			area.getActors().objective_addTaskToStart(actor, objectivePriority.objectiveType->makeFor(area, actor));
			return;
		}
	// No assignable tasks, do an idle task.
	if(!area.getActors().stamina_isFull(actor))
		area.getActors().objective_addTaskToStart(actor, std::make_unique<RestObjective>(area));
	else
		area.getActors().objective_addTaskToStart(actor, std::make_unique<WanderObjective>(actor));
}
void ObjectiveTypePrioritySet::setDelay(Area& area, ObjectiveTypeId objectiveTypeId)
{
	auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType->getObjectiveTypeId() == objectiveTypeId; });
	// If found is not in data it was assigned by some other means (such as player interaction) so we don't need a delay.
	if(found != m_data.end())
		found->doNotAssignAgainUntil = area.m_simulation.m_step + Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective;
}
uint8_t ObjectiveTypePrioritySet::getPriorityFor(ObjectiveTypeId objectiveTypeId) const
{
	const auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType->getObjectiveTypeId() == objectiveTypeId; });
	if(found == m_data.end())
		return 0;
	return found->priority;
}
bool ObjectiveTypePrioritySet::isOnDelay(Area& area, ObjectiveTypeId objectiveTypeId) const
{
	const ObjectivePriority& objectivePriority = getById(objectiveTypeId);
	return objectivePriority.doNotAssignAgainUntil > area.m_simulation.m_step;
}

Step ObjectiveTypePrioritySet::getDelayEndFor(ObjectiveTypeId objectiveTypeId) const
{
	const ObjectivePriority& objectivePriority = getById(objectiveTypeId);
	return objectivePriority.doNotAssignAgainUntil;
}
// SupressedNeed
SupressedNeed::SupressedNeed(Area& area, std::unique_ptr<Objective> o, ActorReference ref) :
	m_objective(std::move(o)), m_event(area.m_eventSchedule), m_actor(ref) { }
SupressedNeed::SupressedNeed(Area& area, const Json& data, DeserializationMemo& deserializationMemo, ActorReference ref) :
	m_event(area.m_eventSchedule), m_actor(ref)
{
	m_objective = deserializationMemo.loadObjective(data["objective"]);
	if(data.contains("eventStart"))
		m_event.schedule(*this, data["eventStart"].get<Step>());
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
	ActorIndex actor = m_actor.getIndex();
	auto objective = std::move(m_objective);
	HasObjectives& hasObjectives = *area.getActors().m_hasObjectives.at(actor);
	hasObjectives.m_supressedNeeds.erase(objective->getObjectiveTypeId());
       	hasObjectives.addNeed(area, std::move(objective)); 
}
SupressedNeedEvent::SupressedNeedEvent(Area& area, SupressedNeed& sn, const Step start) :
	ScheduledEvent(area.m_simulation, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective, start), m_supressedNeed(sn) { }
void SupressedNeedEvent::execute(Simulation&, Area* area) { m_supressedNeed.callback(*area); }
void SupressedNeedEvent::clearReferences(Simulation&, Area*) { m_supressedNeed.m_event.clearPointer(); }
// ObjectiveType.
Json ObjectiveType::toJson() const { return Json{{"type", getObjectiveTypeId()}}; }
// Static method.
void ObjectiveType::load()
{
	objectiveTypes["wood working"] = std::make_unique<CraftObjectiveType>(SkillType::byName("wood working"));
	objectiveTypes["metal working"] = std::make_unique<CraftObjectiveType>(SkillType::byName("metal working"));
	objectiveTypes["stone carving"] = std::make_unique<CraftObjectiveType>(SkillType::byName("stone carving"));
	objectiveTypes["bone carving"] = std::make_unique<CraftObjectiveType>(SkillType::byName("bone carving"));
	objectiveTypes["assembling"] = std::make_unique<CraftObjectiveType>(SkillType::byName("assembling"));
	objectiveTypes["dig"] = std::make_unique<DigObjectiveType>();
	//  TODO: specalize construct into Earthworks, Masonry, Metal Construction, and Carpentry.
	objectiveTypes["construct"] = std::make_unique<ConstructObjectiveType>();
	objectiveTypes["stockpile"] = std::make_unique<StockPileObjectiveType>();
	objectiveTypes["sow seeds"] = std::make_unique<SowSeedsObjectiveType>();
	objectiveTypes["give plants fluid"] = std::make_unique<GivePlantsFluidObjectiveType>();
	objectiveTypes["harvest"] = std::make_unique<HarvestObjectiveType>();
	objectiveTypes["wood cutting"] = std::make_unique<WoodCuttingObjectiveType>();
	for(auto& pair : objectiveTypes)
		objectiveTypeNames[pair.second.get()] = pair.first;
}
inline void to_json(Json& data, const ObjectiveType* const& objectiveType)
{ 
	assert(ObjectiveType::objectiveTypeNames.contains(objectiveType));
	data = ObjectiveType::objectiveTypeNames.at(objectiveType); 
}
inline void from_json(const Json& data, const ObjectiveType*& objectiveType)
{ 
	std::string name = data.get<std::string>();
	assert(ObjectiveType::objectiveTypes.contains(name));
	assert(ObjectiveType::objectiveTypes.at(name).get());
	objectiveType = ObjectiveType::objectiveTypes.at(name).get(); 
}
// Objective.
Objective::Objective(uint32_t p) : m_priority(p) { }
Objective::Objective(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) :
	m_priority(data["priority"].get<uint32_t>()), m_detour(data["detour"].get<bool>()) 
{ 
	deserializationMemo.m_objectives[data["address"].get<uintptr_t>()] = this;
}
Json Objective::toJson() const 
{ 
	return Json{{"type", getObjectiveTypeId()}, {"priority", m_priority}, 
		{"detour", m_detour}, {"address", reinterpret_cast<uintptr_t>(this)}}; 
}
CannotCompleteObjectiveDishonorCallback::CannotCompleteObjectiveDishonorCallback(Area& area, const Json& data) : 
	m_area(area),
	m_actor(area.getActors().getReference(data["actor"].get<ActorIndex>())) { }
Json CannotCompleteObjectiveDishonorCallback::toJson() const
{
	return {{"actor", m_actor.getIndex()}};
}
void CannotCompleteObjectiveDishonorCallback::execute(uint32_t, uint32_t) { m_area.getActors().objective_canNotCompleteSubobjective(m_actor.getIndex()); }
// HasObjectives.
void HasObjectives::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& objective : data["needsQueue"])
	{
		m_needsQueue.push_back(deserializationMemo.loadObjective(objective));
		m_idsOfObjectivesInNeedsQueue.insert(m_needsQueue.back()->getObjectiveTypeId());
	}
	for(const Json& objective : data["tasksQueue"])
		m_tasksQueue.push_back(deserializationMemo.loadObjective(objective));
	if(data.contains("currentObjective"))
		m_currentObjective = deserializationMemo.m_objectives.at(data["currentObjective"].get<uintptr_t>());
	for(const Json& pair : data["supressedNeeds"])
		m_supressedNeeds.try_emplace(pair[0].get<ObjectiveTypeId>(), pair[1], deserializationMemo, m_actor);
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
		Json pair{supressedNeed.first, supressedNeed.second.toJson()};
		data["supressedNeeds"].push_back(pair);
	}
	data["prioritySet"] = m_prioritySet.toJson();
	return data;
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
	ObjectiveTypeId objectiveTypeId = objective->getObjectiveTypeId();
	if(hasSupressedNeed(objectiveTypeId))
		return;
	// Wake up to fulfil need.
	if(!area.getActors().sleep_isAwake(m_actor))
		area.getActors().sleep_wakeUpEarly(m_actor);
	Objective* o = objective.get();
	m_needsQueue.push_back(std::move(objective));
	m_needsQueue.sort([](const std::unique_ptr<Objective>& a, const std::unique_ptr<Objective>& b){ return a->m_priority > b->m_priority; });
	m_idsOfObjectivesInNeedsQueue.insert(objectiveTypeId);
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
	ObjectiveTypeId objectiveTypeId = objective.getObjectiveTypeId();
	bool isCurrent = m_currentObjective == &objective;
	if(m_idsOfObjectivesInNeedsQueue.contains(objectiveTypeId))
	{
		// Remove canceled objective from needs queue.
		auto found = std::ranges::find_if(m_needsQueue, [&](auto& o){ return o->getObjectiveTypeId() == objectiveTypeId; });
		m_needsQueue.erase(found);
		m_idsOfObjectivesInNeedsQueue.erase(objectiveTypeId);
	}
	else
	{
		// Remove canceled objective from task queue.
		assert(m_tasksQueue.end() != std::ranges::find_if(m_tasksQueue, [&](auto& o){ return &objective == o.get(); }));
		std::erase_if(m_tasksQueue, [&](auto& o){ return &objective == o.get(); });
	}
	actors.canReserve_clearAll(m_actor);
	actors.reservable_unreserveAll(m_actor);
	actors.leadAndFollowDisband(m_actor);
	ActorOrItemIndex wasCarrying = area.getActors().canPickUp_putDownIfAny(m_actor, actors.getLocation(m_actor));
	if(wasCarrying.exists() && actors.getFaction(m_actor) != nullptr)
	{
		if(wasCarrying.isItem())
		{
			ItemIndex item = wasCarrying.get();
			const FactionId faction = actors.getFactionId(m_actor);
			if(area.m_hasStockPiles.contains(faction))
				area.m_hasStockPiles.at(faction).addItem(item);
		}
		else
		{
			assert(wasCarrying.isActor());
			//TODO: add to medical listing.
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
	actors.project_unset(m_actor);
	destroy(area, objective);
}
void HasObjectives::objectiveComplete(Area& area, Objective& objective)
{
	Actors& actors = area.getActors();
	assert(objective.getObjectiveTypeId() == ObjectiveTypeId::Wait || actors.sleep_isAwake(m_actor));
	actors.move_pathRequestMaybeCancel(m_actor);
	actors.project_unset(m_actor);
	destroy(area, objective);
}
void HasObjectives::subobjectiveComplete(Area& area)
{
	Actors& actors = area.getActors();
	assert(actors.sleep_isAwake(m_actor));
	if(m_currentObjective == nullptr)
		getNext(area);
	else
		m_currentObjective->execute(area, m_actor);
}
void HasObjectives::cannotCompleteSubobjective(Area& area)
{
	Actors& actors = area.getActors();
	actors.move_clearPath(m_actor);
	//TODO: generate cancelaton message?
	//TODO: mandate existance of objective?
	if(m_currentObjective == nullptr)
		getNext(area);
	else
		m_currentObjective->execute(area, m_actor);
}
Objective& HasObjectives::getCurrent() 
{
	assert(m_currentObjective != nullptr);
       	return *m_currentObjective; 
}
bool HasObjectives::hasTask(ObjectiveTypeId objectiveTypeId) const
{
	return std::ranges::find(m_tasksQueue, objectiveTypeId, [](const auto& objective){ return objective->getObjectiveTypeId(); }) != m_tasksQueue.end();
}
bool HasObjectives::hasNeed(ObjectiveTypeId objectiveTypeId) const
{
	return std::ranges::find(m_needsQueue, objectiveTypeId, [](const auto& objective){ return objective->getObjectiveTypeId(); }) != m_tasksQueue.end();
}
// Does not use ::cancel because needs to move supressed objective into storage.
void HasObjectives::cannotFulfillNeed(Area& area, Objective& objective)
{
	Actors& actors = area.getActors();
	actors.move_clearPath(m_actor);
	actors.canReserve_clearAll(m_actor);
	ObjectiveTypeId objectiveTypeId = objective.getObjectiveTypeId();
	bool isCurrent = m_currentObjective == &objective;
	objective.cancel(area, m_actor);
	auto found = std::ranges::find_if(m_needsQueue, [&](std::unique_ptr<Objective>& o) { return o->getObjectiveTypeId() == objectiveTypeId; });
	assert(found != m_needsQueue.end());
	// Store supressed need.
	m_supressedNeeds.try_emplace(objectiveTypeId, area, std::move(*found), m_actor);
	// Remove from needs queue.
	m_idsOfObjectivesInNeedsQueue.erase(objectiveTypeId);
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
	actors.leadAndFollowDisband(m_actor);
	// Store delay to wait before trying again.
	m_prioritySet.setDelay(area, objective.getObjectiveTypeId());
	cancel(area, objective);
	//TODO: generate cancelation message?
}
void HasObjectives::detour(Area& area)
{
	//TODO: This conditional is required because objectives are not mandated to always exist.
	if(m_currentObjective != nullptr)
		m_currentObjective->detour(area, m_actor);
}
