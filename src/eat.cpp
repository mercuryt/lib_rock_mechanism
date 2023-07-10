EatEvent::EatEvent(delay, EatObjective& eo) : ScheduledEvent(delay), m_eatObjective(eo) {}
void EatEvent::execute()
{
	auto& actor = m_eatObjective.m_actor;
	if(!actor.canEatAt(*actor.m_location))
		m_eatObjective.m_threadedTask.create(m_eatObjective);
	else
	{
		if(m_actor.m_actorType.carnivore)
			for(Actor* actor : block->m_actors)
				if(!actor.m_alive && m_actor.canEat(actor))
					m_actor.eat(*actor);
		if(m_actor.m_actorType.herbavore)
			for(Plant* plant : block->m_plants)
				if(m_actor.canEat(plant))
					m_actor.eat(*plant);
		actor.finishedCurrentObjective();
	}
}
HungerEvent::
EatThreadedTask::EatThreadedTask(EatObjective& eo) : m_fluidType(ft), m_eatObjective(eo), m_blockResult(nullptr), m_huntResult(nullptr) {}

void EatThreadedTask::readStep()
{
	constexpr maxRankedEatDesire = 3;
	std::array<Block*, maxRankedEatDesire> candidates = {};
	auto destinationCondition = [&](Block* block)
	{
		uint32_t eatDesire = m_eatObjective.m_actor.m_mustEat.getDesireToEatSomethingAt(*block);
		if(eatDesire == UINT32_MAX)
			return true;
		--eatDesire;
		if(candidates[eatDesire] == nullptr)
			candidates[eatDesire] = block;
		return false;
	}
	m_blockResult = util::getForActorToPredicateReturnEndOnly(pathCondition, destinationCondition, *m_eatObjective.m_actor.m_location, Config::maxBlocksToLookForBetterFood);
	if(m_blockResult == nullptr)
		for(size_t i = 0; i < maxRankedEatDesire; ++i)
			if(candidates[i] != nullptr)
			{
				m_blockResult = candidates[i];
				continue;
			}
	// Nothing to scavenge or graze, maybe hunt.
	if(m_blockResult == nullptr && m_actor.m_actorType.carnivore)
	{
		auto huntCondition = [&](Block* block)
		{
			for(Actor& actor : block->m_actors)
				if(m_actor.canEat(actor))
					return true;
			return false;
		}
		m_huntResult = util::getForActorToPredicateReturnEndOnly(pathCondition, huntCondition, *m_eatObjective.m_actor.m_location);
	}
}
void EatThreadedTask::writeStep()
{
	if(m_blockResult == nullptr)
	{
		if(m_huntResult == nullptr)
			m_eatObjective.m_actor.onNothingToEat();
		else
		{
			std::unique_ptr<Objective> killObjective = std::make_unique<KillObjective>(Config::eatPriority + 1, *m_huntResult);
			m_eatObjective.m_actor.addObjective(std::move(killObjective));
		}
	}
	else
	{
		m_eatObjective.m_actor.setDestination(m_result);
		//TODO: reserve food if sentient.
	}


}
EatObjective::EatObjective(Actor& a) : Objective(Config::eatPriority), m_actor(a), m_foodLocation(nullptr), m_foodItem(nullptr) { }
void EatObjective::execute()
{
	if(m_foodLocation == nullptr)
		m_threadedTask.create(*this);
	else
	{	
		Block* eatingLocation = m_actor.m_mustEat.m_eatingLocation; 
		// Civilized eating.
		if(m_actor.isSentient() && eatingLocation != nullptr && m_foodItem != nullptr)
		{
			// Has meal
			if(m_actor.m_hasItems.isCarrying(m_foodItem))
			{
				// Is at eating location.
				if(m_actor.m_location == eatingLocation)
					m_eatEvent.schedule(Config::stepsToEat, m_foodItem);
				else
					m_actor.setDestination(eatingLocation);
			}
			else
			{
				// Is at pickup location.
				if(m_actor.m_location == m_foodLocation)
				{
					m_actor.m_hasItems.pickUp(m_foodItem);
					execute();
				}
				else
					m_actor.setDestination(m_foodLocation);
			}

		}
		// Uncivilized eating.
		else
		{
			if(m_actor.m_location == m_foodLocation)
			{
				if(m_foodItem != nullptr)
					m_eatEvent.schedule(Config::stepsToEat, m_foodItem);
				else
					m_eatEvent.schedule(Config::stepsToEat);
			}
			else
				m_actor.setDestination(m_foodLocation);
		}
	}
}
bool EatObjective::canEatAt(Block& block)
{
	for(Item* item : m_block.m_hasItems.get())
	{
		if(m_actor.m_mustEat.canEat(item))
			return true;
		if(item->itemType.internalVolume != 0)
			for(Item* i : item->containsItemsOrFluids.getItems())
				if(m_actor.m_mustEat.canEat(i))
					return true;
	}
	if(m_actor.m_actorType.carnivore)
		for(Actor* other : block->m_actors)
			if(!other.m_alive && m_actor.canEat(actor))
				return true;
	if(m_actor.m_actorType.herbavore)
		for(Plant* plant : block->m_plants)
			if(m_actor.canEat(plant))
				return true;
	return false;
}
MustEat::MustEat(Actor& a) : m_actor(a), m_massFoodRequested(0) { }
void MustEat::eat(uint32_t mass)
{
	assert(mass <= m_massFoodRequested);
	m_massFoodRequested -= mass;
	m_hungerEvent.unschedule();
	if(m_massFoodRequested == 0)
		m_actor.m_growing.maybeStart();
	else
	{
		uint32_t delay = util::scaleByInverseFraction(m_stepsTillDieWithoutFood, m_massFoodRequested, massFoodForBodyMass());
		makeHungerEvent(delay);
		m_hasObjectives.addNeed(std::make_unique<EatObjective>(*this));
	}
}
void MustEat::setNeedsFood()
{
	if(!m_hungerEvent.empty())
		m_actor.die(CauseOfDeath("hunger"));
	else
	{
		makeHungerEvent(m_stepsTillDieWithoutFood);
		m_actor.m_growing.stop();
		m_massFoodRequested = massFoodForBodyMass();
		createFoodRequest();
	}
}
uint32_t MustEat::massFoodForBodyMass() const
{
	return m_actor.getMass() / Config::unitsOfFoodRequestedPerUnitOfBodyMass;
}
const uint32_t& MustEat::getMassFoodRequested() const { return m_massFoodRequested; }
const uint32_t& MustEat::getPercentStarved() const
{
	if(m_hungerEvent.empty())
		return 0;
	else
		return m_hungerEvent.percentComplete();
}
uint32_t MustEat::getDesireToEatSomethingAt(Block* block) const
{
	for(Item* item : block->m_hasItems.get())
		if(item.m_isPreparedMeal)
			return UINT32_MAX;
	if(m_actor.m_mustEat.grazesFruit())
		for(Plant* plant : block->m_hasPlants.get())
			if(plant->m_percentFruit != 0)
				return 1;
	if(m_actor.m_mustEat.scavengesCarcasses())
		for(Actor* actor : block->m_containsActors.get())
			if(!actor->m_isAlive && actor.getFluidType() == m_actor.getFluidType())
				return 2;
	if(m_actor.m_mustEat.grazesLeaves())
		for(Plant* plant : block->m_hasPlants.get())
			if(plant->m_percentFoliage != 0)
				return 3;
	return 0;
}
