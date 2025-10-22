#include "actors.h"
#include "../eat.h"
#include "../objectives/eat.h"
void Actors::eat_do(const ActorIndex& index, const Mass& mass)
{
	m_mustEat[index]->eat(m_area, mass);
}
void Actors::eat_setIsHungry(const ActorIndex& index)
{
	m_mustEat[index]->setNeedsFood(m_area);
}
void Actors::eat_setNeverHungry(const ActorIndex& index)
{
	m_mustEat[index]->unschedule();
}
bool Actors::eat_isHungry(const ActorIndex& index) const
{
	return m_mustEat[index]->needsFood();
}
bool Actors::eat_isEating(const ActorIndex& index) const
{
	if(objective_getCurrentName(index) != "eat")
		return false;
	return objective_getCurrent<EatObjective>(index).hasEvent();
}
bool Actors::eat_canEatActor(const ActorIndex& index, const ActorIndex& other) const
{
	return m_mustEat[index]->canEatActor(m_area, other);
}
bool Actors::eat_canEatItem(const ActorIndex& index, const ItemIndex& item) const
{
	return m_mustEat[index]->canEatItem(m_area, item);
}
bool Actors::eat_canEatPlant(const ActorIndex& index, const PlantIndex& plant) const
{
	return m_mustEat[index]->canEatPlant(m_area, plant);
}
Percent Actors::eat_getPercentStarved(const ActorIndex& index) const
{
	return m_mustEat[index]->getPercentStarved();
}
Point3D Actors::eat_getOccupiedOrAdjacentPointWithTheMostDesiredFood(const ActorIndex& index) const
{
	return m_mustEat[index]->getOccupiedOrAdjacentPointWithHighestDesireFoodOfAcceptableDesireability(m_area);
}
std::pair<Point3D, uint8_t> Actors::eat_getDesireToEatSomethingAt(const ActorIndex& index, const Cuboid& cuboid) const
{
	return m_mustEat[index]->getDesireToEatSomethingAt(m_area, cuboid);
}
uint8_t Actors::eat_getMinimumAcceptableDesire(const ActorIndex& index) const
{
	return m_mustEat[index]->getMinimumAcceptableDesire(m_area);
}
bool Actors::eat_hasObjective(const ActorIndex& index) const
{
	return m_mustEat[index]->hasObjective();
}
Mass Actors::eat_getMassFoodRequested(const ActorIndex& index) const
{
	return m_mustEat[index]->getMassFoodRequested();
}
Step Actors::eat_getHungerEventStep(const ActorIndex& index) const
{
	return m_mustEat[index]->getHungerEventStep();
}
bool Actors::eat_hasHungerEvent(const ActorIndex& index) const
{
	return m_mustEat[index]->hasHungerEvent();
}
