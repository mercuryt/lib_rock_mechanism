//ObjectiveTypePrioritySet
#include "objective.h"
#include "actorOrItemIndex.h"
#include "area.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include "simulation/hasActors.h"
#include "objectives/givePlantsFluid.h"
#include "objectives/harvest.h"
#include "objectives/rest.h"
#include "objectives/sowSeeds.h"
#include "objectives/wander.h"
#include "woodcutting.h"
#include "craft.h"
#include "dig.h"
#include "construct.h"
#include "stockpile.h"

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
void ObjectiveTypePrioritySet::setPriority(Area& area, const ObjectiveType& objectiveType, uint8_t priority)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& x) { return x.objectiveType == &objectiveType; });
	if(found == m_data.end())
		m_data.emplace_back(&objectiveType, priority, 0); // waitUntillStepBeforeAssigningAgain initilizes to 0.
	else
		found->priority = priority;
	std::ranges::sort(m_data, std::ranges::greater{}, &ObjectivePriority::priority);
	area.m_actors.objective_maybeDoNext(m_actor);
}
void ObjectiveTypePrioritySet::remove(const ObjectiveType& objectiveType)
{
	std::erase_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == &objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Area& area, ActorIndex actor)
{
	assert(!area.m_actors.objective_exists(actor));
	assert(!area.m_actors.objective_queuesAreEmpty(actor));
	Step currentStep = area.m_simulation.m_step;
	for(auto& objectivePriority : m_data)
		if(currentStep > objectivePriority.doNotAssignAgainUntil && objectivePriority.objectiveType->canBeAssigned(area, actor))
		{
			area.m_actors.objective_addTaskToStart(actor, objectivePriority.objectiveType->makeFor(area, actor));
			return;
		}
	// No assignable tasks, do an idle task.
	if(!area.m_actors.stamina_isFull(actor))
		area.m_actors.objective_addTaskToStart(actor, std::make_unique<RestObjective>(area, actor));
	else
		area.m_actors.objective_addTaskToStart(actor, std::make_unique<WanderObjective>(area, actor));
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
SupressedNeed::SupressedNeed(Area& area, std::unique_ptr<Objective> o, ActorIndex a) :
	m_area(area), m_actor(a), m_objective(std::move(o)), m_event(area.m_simulation.m_eventSchedule) { }
/*
SupressedNeed::SupressedNeed(const Json& data, DeserializationMemo& deserializationMemo, ActorIndex a) :
	m_actor(a), m_event(deserializationMemo.m_simulation.m_eventSchedule)
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
*/
void SupressedNeed::callback() 
{
	auto objective = std::move(m_objective);
	HasObjectives& hasObjectives = m_area.m_actors.m_hasObjectives.at(m_actor);
	hasObjectives.m_supressedNeeds.erase(objective->getObjectiveTypeId());
       	hasObjectives.addNeed(m_area, std::move(objective)); 
}
SupressedNeedEvent::SupressedNeedEvent(Area& area, SupressedNeed& sn, const Step start) :
	ScheduledEvent(area.m_simulation, Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective, start), m_supressedNeed(sn) { }
void SupressedNeedEvent::execute() { m_supressedNeed.callback(); }
void SupressedNeedEvent::clearReferences() { m_supressedNeed.m_event.clearPointer(); }
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
Objective::Objective(ActorIndex a, uint32_t p) : m_actor(a), m_priority(p) {}
/*
Objective::Objective(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) :
	m_actor(deserializationMemo.m_simulation.m_hasActors->getById(data["actor"].get<ActorId>())), m_priority(data["priority"].get<uint32_t>()), m_detour(data["detour"].get<bool>()) 
{ 
	deserializationMemo.m_objectives[data["address"].get<uintptr_t>()] = this;
}
Json Objective::toJson() const 
{ 
	// TODO: Why do we have to specify actor id here?
	return Json{{"type", getObjectiveTypeId()}, {"actor", m_actor.m_id}, {"priority", m_priority}, 
		{"detour", m_detour}, {"address", reinterpret_cast<uintptr_t>(this)}}; 
}
CannotCompleteObjectiveDishonorCallback::CannotCompleteObjectiveDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo) : 
	m_actor(deserializationMemo.m_simulation.m_hasActors->getById(data["actor"].get<ActorId>())) { }
Json CannotCompleteObjectiveDishonorCallback::toJson() const
{
	//TODO: We should not have to specify actor id.
	return {{"actor", m_actor.m_id}};
}
*/
void CannotCompleteObjectiveDishonorCallback::execute(uint32_t, uint32_t) { m_area.m_actors.objective_canNotCompleteSubobjective(m_actor); }
// HasObjectives.
HasObjectives::HasObjectives(ActorIndex a) : m_actor(a), m_prioritySet(a) { }
/*
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
*/
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
				m_currentObjective->delay();
			else
				m_currentObjective->cancel();
			setCurrentObjective(area, objective);
		}
	}
}
void HasObjectives::setCurrentObjective(Area& area, Objective& objective)
{
	area.m_actors.move_clearPath(m_actor);
	area.m_actors.project_maybeClear(m_actor);
	m_currentObjective = &objective;
	objective.execute();
}
void HasObjectives::addNeed(Area& area, std::unique_ptr<Objective> objective)
{
	ObjectiveTypeId objectiveTypeId = objective->getObjectiveTypeId();
	if(hasSupressedNeed(objectiveTypeId))
		return;
	// Wake up to fulfil need.
	if(!area.m_actors.isAwake(m_actor))
		area.m_actors.wakeUpEarly(m_actor);
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
	area.m_actors.cancelAllReservationsMadeBy(m_actor);
	area.m_actors.cancelAllReservationsMadeOn(m_actor);
	area.m_actors.maybeDisbandLeadAndFollow(m_actor);
	ActorOrItemIndex wasCarrying = area.m_actors.canPickUp_putDownIfAny(m_actor, area.m_actors.getLocation(m_actor));
	HasShape* wasCarrying = m_actor.m_canPickup.putDownIfAny(m_actor.m_location);
	if(wasCarrying != nullptr && m_actor.getFaction() != nullptr)
	{
		if(wasCarrying->isItem())
		{
			Item& item = static_cast<Item&>(*wasCarrying);
			Faction& faction = *m_actor.getFaction();
			if(m_actor.m_area->m_hasStockPiles.contains(faction))
				m_actor.m_area->m_hasStockPiles.at(faction).addItem(item);
		}
		else
		{
			assert(wasCarrying->isActor());
			//TODO: add to medical listing.
		}
	}
	if(isCurrent)
		getNext();
}
void HasObjectives::cancel(Objective& objective)
{
	objective.cancel();
	m_actor.m_canMove.maybeCancelThreadedTask();
	m_actor.m_project = nullptr;
	destroy(objective);
}
void HasObjectives::objectiveComplete(Objective& objective)
{
	assert(objective.getObjectiveTypeId() == ObjectiveTypeId::Wait || m_actor.m_mustSleep.isAwake());
	m_actor.m_canMove.maybeCancelThreadedTask();
	m_actor.m_project = nullptr;
	destroy(objective);
}
void HasObjectives::subobjectiveComplete()
{
	assert(m_actor.m_mustSleep.isAwake());
	if(m_currentObjective == nullptr)
		getNext();
	else
		m_currentObjective->execute();
}
void HasObjectives::cannotCompleteSubobjective()
{
	m_actor.m_canMove.clearPath();
	//TODO: generate cancelaton message?
	//TODO: mandate existance of objective?
	if(m_currentObjective == nullptr)
		getNext();
	else
		m_currentObjective->execute();
}
Objective& HasObjectives::getCurrent() 
{
	assert(m_currentObjective != nullptr);
       	return *m_currentObjective; 
}
// Does not use ::cancel because needs to move supressed objective into storage.
void HasObjectives::cannotFulfillNeed(Objective& objective)
{
	m_actor.m_canMove.clearPath();
	m_actor.m_canReserve.deleteAllWithoutCallback();
	ObjectiveTypeId objectiveTypeId = objective.getObjectiveTypeId();
	bool isCurrent = m_currentObjective == &objective;
	objective.cancel();
	auto found = std::ranges::find_if(m_needsQueue, [&](std::unique_ptr<Objective>& o) { return o->getObjectiveTypeId() == objectiveTypeId; });
	assert(found != m_needsQueue.end());
	// Store supressed need.
	m_supressedNeeds.try_emplace(objectiveTypeId, std::move(*found), m_actor);
	// Remove from needs queue.
	m_idsOfObjectivesInNeedsQueue.erase(objectiveTypeId);
	m_needsQueue.erase(found);
	// No need to disband leadAndFollow here, needs don't use it.
	m_actor.m_canMove.maybeCancelThreadedTask();
	if(isCurrent)
		getNext();
}
void HasObjectives::cannotFulfillObjective(Objective& objective)
{
	m_actor.m_canMove.clearPath();
	m_actor.m_canReserve.deleteAllWithoutCallback();
	m_actor.m_canFollow.maybeDisband();
	// Store delay to wait before trying again.
	m_prioritySet.setDelay(objective.getObjectiveTypeId());
	cancel(objective);
	//TODO: generate cancelation message?
}
void HasObjectives::detour()
{
	//TODO: This conditional is required because objectives are not mandated to always exist.
	if(m_currentObjective != nullptr)
		m_currentObjective->detour();
}
