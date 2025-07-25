#include "actors.h"
#include "../eat.h"
void Actors::eat_do(const ActorIndex& index, const Mass& mass)
{
	m_mustEat[index]->eat(m_area, mass);
}
void Actors::eat_setIsHungry(const ActorIndex& index)
{
	m_mustEat[index]->setNeedsFood(m_area);
}
bool Actors::eat_isHungry(const ActorIndex& index) const
{
	return m_mustEat[index]->needsFood();
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
Point3D Actors::eat_getAdjacentPointWithTheMostDesiredFood(const ActorIndex& index) const
{
	return m_mustEat[index]->getAdjacentPointWithHighestDesireFoodOfAcceptableDesireability(m_area);
}
uint32_t Actors::eat_getDesireToEatSomethingAt(const ActorIndex& index, const Point3D& point) const
{
	return m_mustEat[index]->getDesireToEatSomethingAt(m_area, point);
}
uint32_t Actors::eat_getMinimumAcceptableDesire(const ActorIndex& index) const
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
