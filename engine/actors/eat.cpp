#include "actors.h"
#include "../eat.h"
void Actors::eat_do(ActorIndex index, Mass mass)
{
	m_mustEat[index]->eat(m_area, mass);
}
bool Actors::eat_isHungry(ActorIndex index) const
{
	return m_mustEat[index]->needsFood();
}
bool Actors::eat_canEatActor(ActorIndex index, ActorIndex other) const
{
	return m_mustEat[index]->canEatActor(m_area, other);
}
bool Actors::eat_canEatItem(ActorIndex index, ItemIndex item) const
{
	return m_mustEat[index]->canEatItem(m_area, item);
}
bool Actors::eat_canEatPlant(ActorIndex index, PlantIndex plant) const
{
	return m_mustEat[index]->canEatPlant(m_area, plant);
}
Percent Actors::eat_getPercentStarved(ActorIndex index) const
{
	return m_mustEat[index]->getPercentStarved();
}
uint32_t Actors::eat_getDesireToEatSomethingAt(ActorIndex index, BlockIndex block) const
{
	return m_mustEat[index]->getDesireToEatSomethingAt(m_area, block);
}
bool Actors::eat_hasObjective(ActorIndex index) const
{
	return m_mustEat[index]->hasObjecive();
}
Mass Actors::eat_getMassFoodRequested(ActorIndex index) const
{
	return m_mustEat[index]->getMassFoodRequested();
}
Step Actors::eat_getHungerEventStep(ActorIndex index) const
{
	return m_mustEat[index]->getHungerEventStep();
}
