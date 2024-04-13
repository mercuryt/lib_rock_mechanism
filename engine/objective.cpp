//ObjectiveTypePrioritySet
#include "objective.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include "actor.h"
#include "stamina.h"
#include "sowSeeds.h"
#include "givePlantsFluid.h"
#include "harvest.h"
#include "wander.h"
#include <cstdio>
#include <numbers>
// Input.
void ObjectiveTypeSetPriorityInputAction::execute()
{
	m_actor.m_hasObjectives.m_prioritySet.setPriority(m_objectiveType, m_priority);
}
void ObjectiveTypeRemoveInputAction::execute()
{
	m_actor.m_hasObjectives.m_prioritySet.remove(m_objectiveType);
}
void ObjectiveTypePrioritySet::load(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo)
{
	m_data = data["data"].get<std::vector<ObjectivePriority>>();
}
Json ObjectiveTypePrioritySet::toJson() const
{
	return Json{{"data", m_data}};
}
void ObjectiveTypePrioritySet::setPriority(const ObjectiveType& objectiveType, uint8_t priority)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& x) { return x.objectiveType == &objectiveType; });
	if(found == m_data.end())
		m_data.emplace_back(&objectiveType, priority, 0); // waitUntillStepBeforeAssigningAgain initilizes to 0.
	else
		found->priority = priority;
	std::ranges::sort(m_data, std::ranges::greater{}, &ObjectivePriority::priority);
	if(m_actor.m_hasObjectives.m_currentObjective == nullptr)
		m_actor.m_hasObjectives.getNext();
}
void ObjectiveTypePrioritySet::remove(const ObjectiveType& objectiveType)
{
	std::erase_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType == &objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Actor& actor)
{
	assert(actor.m_hasObjectives.m_currentObjective == nullptr);
	assert(actor.m_hasObjectives.m_needsQueue.empty());
	assert(actor.m_hasObjectives.m_tasksQueue.empty());
	Step currentStep = actor.getSimulation().m_step;
	for(auto& objectivePriority : m_data)
		if(currentStep > objectivePriority.doNotAssignAgainUntil && objectivePriority.objectiveType->canBeAssigned(actor))
		{
			actor.m_hasObjectives.addTaskToStart(objectivePriority.objectiveType->makeFor(actor));
			return;
		}
	// No assignable tasks, do an idle task.
	if(!actor.m_stamina.isFull())
		actor.m_hasObjectives.addTaskToStart(std::make_unique<RestObjective>(actor));
	else
		actor.m_hasObjectives.addTaskToStart(std::make_unique<WanderObjective>(actor));
}
void ObjectiveTypePrioritySet::setDelay(ObjectiveTypeId objectiveTypeId)
{
	auto found = std::ranges::find_if(m_data, [&](ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType->getObjectiveTypeId() == objectiveTypeId; });
	// If found is not in data it was assigned by some other means (such as player interaction) so we don't need a delay.
	if(found != m_data.end())
		found->doNotAssignAgainUntil = m_actor.getSimulation().m_step + Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective;
}
uint8_t ObjectiveTypePrioritySet::getPriorityFor(ObjectiveTypeId objectiveTypeId) const
{
	auto found = std::ranges::find_if(m_data, [&](const ObjectivePriority& objectivePriority){ return objectivePriority.objectiveType->getObjectiveTypeId() == objectiveTypeId; });
	if(found == m_data.end())
		return 0;
	return found->priority;
}
// SupressedNeed
SupressedNeed::SupressedNeed(std::unique_ptr<Objective> o, Actor& a) : m_actor(a), m_objective(std::move(o)), m_event(a.getSimulation().m_eventSchedule) { }
SupressedNeed::SupressedNeed(const Json& data, DeserializationMemo& deserializationMemo, Actor& a) : m_actor(a), m_event(deserializationMemo.m_simulation.m_eventSchedule)
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
void SupressedNeed::callback() 
{
	auto objective = std::move(m_objective);
	m_actor.m_hasObjectives.m_supressedNeeds.erase(objective->getObjectiveTypeId());
       	m_actor.m_hasObjectives.addNeed(std::move(objective)); 
}
SupressedNeedEvent::SupressedNeedEvent(SupressedNeed& sn, const Step start) : ScheduledEvent(sn.m_actor.getSimulation(), Config::stepsToDelayBeforeTryingAgainToCompleteAnObjective, start), m_supressedNeed(sn) { }
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
	objectiveTypes["construct"] = std::make_unique<ConstructObjectiveType>();
	objectiveTypes["stockpile"] = std::make_unique<StockPileObjectiveType>();
	objectiveTypes["sow seeds"] = std::make_unique<SowSeedsObjectiveType>();
	objectiveTypes["give plants fluid"] = std::make_unique<GivePlantsFluidObjectiveType>();
	objectiveTypes["harvest"] = std::make_unique<HarvestObjectiveType>();
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
Objective::Objective(Actor& a, uint32_t p) : m_actor(a), m_priority(p) {}
Objective::Objective(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) :
	m_actor(deserializationMemo.m_simulation.getActorById(data["actor"].get<ActorId>())), m_priority(data["priority"].get<uint32_t>()), m_detour(data["detour"].get<bool>()) 
{ 
	deserializationMemo.m_objectives[data["address"].get<uintptr_t>()] = this;
}
Json Objective::toJson() const 
{ 
	return Json{{"type", getObjectiveTypeId()}, {"actor", m_actor}, {"priority", m_priority}, 
		{"detour", m_detour}, {"address", reinterpret_cast<uintptr_t>(this)}}; 
}
// HasObjectives.
HasObjectives::HasObjectives(Actor& a) : m_actor(a), m_currentObjective(nullptr), m_prioritySet(a) { }
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
void HasObjectives::getNext()
{
	m_currentObjective = nullptr;
	if(m_needsQueue.empty())
	{
		if(m_tasksQueue.empty())
			m_prioritySet.setObjectiveFor(m_actor);
		else
			setCurrentObjective(*m_tasksQueue.front().get());
	}
	else
	{
		if(m_tasksQueue.empty())
			setCurrentObjective(*m_needsQueue.front().get());
		else
		{
			if(m_tasksQueue.front()->m_priority > m_needsQueue.front()->m_priority)
				setCurrentObjective(*m_tasksQueue.front().get());
			else
				setCurrentObjective(*m_needsQueue.front().get());
		}
	}
}
void HasObjectives::maybeUsurpsPriority(Objective& objective)
{
	if(m_currentObjective == nullptr)
		setCurrentObjective(objective);
	else
	{
		if(m_currentObjective->m_priority < objective.m_priority)
		{
			if(m_currentObjective->canResume())
				m_currentObjective->delay();
			else
				m_currentObjective->cancel();
			setCurrentObjective(objective);
		}
	}
}
void HasObjectives::setCurrentObjective(Objective& objective)
{
	m_actor.m_canMove.clearPath();
	m_actor.m_project = nullptr;
	m_currentObjective = &objective;
	objective.execute();
}
void HasObjectives::addNeed(std::unique_ptr<Objective> objective)
{
	ObjectiveTypeId objectiveTypeId = objective->getObjectiveTypeId();
	if(hasSupressedNeed(objectiveTypeId))
		return;
	// Wake up to fulfil need.
	if(!m_actor.m_mustSleep.isAwake())
		m_actor.m_mustSleep.wakeUpEarly();
	Objective* o = objective.get();
	m_needsQueue.push_back(std::move(objective));
	m_needsQueue.sort([](const std::unique_ptr<Objective>& a, const std::unique_ptr<Objective>& b){ return a->m_priority > b->m_priority; });
	m_idsOfObjectivesInNeedsQueue.insert(objectiveTypeId);
	maybeUsurpsPriority(*o);
}
void HasObjectives::addTaskToEnd(std::unique_ptr<Objective> objective)
{
	Objective* o = objective.get();
	m_tasksQueue.push_back(std::move(objective));
	if(m_currentObjective == nullptr)
		setCurrentObjective(*o);
}
void HasObjectives::addTaskToStart(std::unique_ptr<Objective> objective)
{
	Objective* o = objective.get();
	m_tasksQueue.push_front(std::move(objective));
	maybeUsurpsPriority(*o);
}
void HasObjectives::replaceTasks(std::unique_ptr<Objective> objective)
{
	m_currentObjective = nullptr;
	m_tasksQueue.clear();
	addTaskToStart(std::move(objective));
}
// Called when an objective is canceled by the player, canceled by the actor, or completed sucessfully.
// TODO: seperate functions for tasks and needs?
void HasObjectives::destroy(Objective& objective)
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
	m_actor.m_canReserve.deleteAllWithoutCallback();
	HasShape* wasCarrying = m_actor.m_canPickup.putDownIfAny(*m_actor.m_location);
	if(wasCarrying != nullptr && m_actor.getFaction() != nullptr)
	{
		if(wasCarrying->isItem())
		{
			Item& item = static_cast<Item&>(*wasCarrying);
			const Faction& faction = *m_actor.getFaction();
			if(m_actor.m_location->m_area->m_hasStockPiles.contains(faction))
				m_actor.m_location->m_area->m_hasStockPiles.at(faction).addItem(item);
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
	m_actor.m_canReserve.deleteAllWithoutCallback();
	m_actor.m_canMove.maybeCancelThreadedTask();
	if(isCurrent)
		getNext();
}
void HasObjectives::cannotFulfillObjective(Objective& objective)
{
	m_actor.m_canMove.clearPath();
	m_actor.m_canReserve.deleteAllWithoutCallback();
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
