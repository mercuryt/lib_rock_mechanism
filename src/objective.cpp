//ObjectiveTypePrioritySet
void ObjectiveTypePrioritySet::setPriority(const ObjectiveType<Actor>& objectiveType, uint8_t priority)
{
	if(m_map.contains(&objectiveType))
	{
		auto& found = std::ranges::find_if(m_vector, [&](const ObjectiveType* ot){ return ot == &objectiveType; });
		assert(found != m_vector.end());
		found.second = priority;
	}
	else
		m_vector.push_back(&objectiveType, priority);
	m_map[&objectiveType] = priority;
	// Sort by priority stored in m_map, high to low.
	std::ranges::sort(m_vector, [&](auto& a, auto& b){ return m_map.at(a) > m_map.at(b); });
}
void ObjectiveTypePrioritySet::remove(const ObjectiveType<Actor>& objectiveType)
{
	m_map.remove(&objectiveType);
	std::erase_if(m_vector, [&](const ObjectiveType* ot){ return ot == &objectiveType; });
}
void ObjectiveTypePrioritySet::setObjectiveFor(Actor& actor)
{
	assert(actor.m_hasObjectives.m_currentObjective == nullptr);
	assert(actor.m_hasObjectives.m_needsQueue.empty());
	assert(actor.m_hasObjectives.m_tasksQueue.empty());
	for(auto& objectiveType : m_vector)
		if(objectiveType->canBeAssigned(actor))
		{
			actor.m_hasObjectives.addTaskToStart(objectiveType.makeFor(actor));
			return;
		}
	actor.m_eventPublisher.publish(PublishedEventType::NoObjectivesAvailible);
}
// Objective.
Objective::Objective(uint32_t p) : m_priority(p) {}
// HasObjectives.
void HasObjectives::getNext()
{
	m_currentObjective = nullptr;
	if(m_needsQueue.empty())
	{
		if(m_tasksQueue.empty())
			m_prioritySet.setObjectiveFor(m_actor);
		else
			setCurrentObjective(m_tasksQueue.front());
	}
	else
	{
		if(m_tasksQueue.empty())
			setCurrentObjective(m_needsQueue.top());
		else
		{
			if(m_tasksQueue.front().priority > m_needsQueue.top().priority)
				setCurrentObjective(m_tasksQueue.front());
			else
				setCurrentObjective(m_needsQueue.top());
		}
	}
}
void HasObjectives::maybeUsurpsPriority(Objective& objective)
{
	if(m_currentObjective == nullptr)
		setCurrentObjective(objective);
	else
	{
		if(m_currentObjective.priority < objective.priority)
		{
			m_currentObjective.interupt();
			setCurrentObjective(objective);
		}
	}
}
void HasObjectives::setCurrentObjective(Objective& objective)
{
	m_currentObjective = objective;
	objective.execute();
}
HasObjectives::HasObjectives(Actor& a) : m_actor(a), m_currentObjective(nullptr) { }
void HasObjectives::addNeed(std::unique_ptr<Objective>& objective)
{
	Objective& o = objective.get();
	m_needsQueue.insert(std::move(objective));
	maybeUsurpsPriority(o);
}
void HasObjectives::addTaskToEnd(std::unique_ptr<Objective>& objective)
{
	Objective& o = objective.get();
	m_tasksQueue.push_back(std::move(objective));
	if(m_currentObjective == nullptr)
		setCurrentObjective(o);
}
void HasObjectives::addTaskToStart(std::unique_ptr<Objective>& objective)
{
	Objective& o = objective.get();
	m_tasksQueue.push_front(std::move(objective));
	maybeUserpsPriority(o);
}
void HasObjectives::cancel(Objective& objective)
{
	auto found = std::find_if(m_needsQueue.begin(), m_needsQueue.end(), [&](auto& o){ return &objective == &o; });
	if(found)
		m_needsQueue.erase(found);
	else
		std::erase_if(m_tasksQueue, [&](auto& o){ return &objective == &o; });
	if(m_currentObjective == &objective)
		getNext();
}
void HasObjectives::objectiveComplete()
{
	getNext();
}
void HasObjectives::taskComplete()
{
	m_currentObjective.execute();
}
