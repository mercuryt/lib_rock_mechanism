#include "actors.h"
#include "../eat.h"
#include "../objectives/eat.h"
void Actors::eat_do(ActorIndex index, const Mass mass)
{
	m_mustEat[index]->eat(m_area, mass);
}
void Actors::eat_setIsHungry(ActorIndex index)
{
	m_mustEat[index]->setNeedsFood(m_area);
}
void Actors::eat_setNeverHungry(ActorIndex index)
{
	m_mustEat[index]->unschedule();
}
bool Actors::eat_isHungry(ActorIndex index) const
{
	return m_mustEat[index]->needsFood();
}
bool Actors::eat_isEating(ActorIndex index) const
{
	if(objective_getCurrentName(index) != "eat")
		return false;
	return objective_getCurrent<EatObjective>(index).hasEvent();
}
bool Actors::eat_canEatActor(ActorIndex index, ActorIndex other) const
{
	return m_mustEat[index]->canEatActor(m_area, other);
}
bool Actors::eat_canEatItem(ActorIndex index, const ItemIndex item) const
{
	return m_mustEat[index]->canEatItem(m_area, item);
}
bool Actors::eat_canEatPlant(ActorIndex index, const PlantIndex plant) const
{
	return m_mustEat[index]->canEatPlant(m_area, plant);
}
Percent Actors::eat_getPercentStarved(ActorIndex index) const
{
	return m_mustEat[index]->getPercentStarved();
}
Point3D Actors::eat_getOccupiedOrAdjacentPointWithTheMostDesiredFood(ActorIndex index) const
{
	return m_mustEat[index]->getOccupiedOrAdjacentPointWithHighestDesireFoodOfAcceptableDesireability(m_area);
}
std::pair<Point3D, int> Actors::eat_getDesireToEatSomethingAt(ActorIndex index, const Cuboid cuboid) const
{
	return m_mustEat[index]->getDesireToEatSomethingAt(m_area, cuboid);
}
int Actors::eat_getMinimumAcceptableDesire(ActorIndex index) const
{
	return m_mustEat[index]->getMinimumAcceptableDesire(m_area);
}
bool Actors::eat_hasObjective(ActorIndex index) const
{
	return m_mustEat[index]->hasObjective();
}
Mass Actors::eat_getMassFoodRequested(ActorIndex index) const
{
	return m_mustEat[index]->getMassFoodRequested();
}
Step Actors::eat_getHungerEventStep(ActorIndex index) const
{
	return m_mustEat[index]->getHungerEventStep();
}
bool Actors::eat_hasHungerEvent(ActorIndex index) const
{
	return m_mustEat[index]->hasHungerEvent();
}
