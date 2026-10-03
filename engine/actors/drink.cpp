#include "actors.h"
#include "../drink.h"
void Actors::drink_do(ActorIndex index, const CollisionVolume volume)
{
	m_mustDrink[index]->drink(m_area, volume);
}
void Actors::drink_setNeedsFluid(ActorIndex index)
{
	m_mustDrink[index]->unschedule();
	m_mustDrink[index]->setNeedsFluid(m_area);
}
void Actors::drink_setNeverThirsty(ActorIndex index)
{
	m_mustDrink[index]->unschedule();
}
CollisionVolume Actors::drink_getVolumeOfFluidRequested(ActorIndex index) const
{
	return m_mustDrink[index]->getVolumeFluidRequested();
}
bool Actors::drink_isThirsty(ActorIndex index) const
{
	return m_mustDrink[index]->needsFluid();
}
FluidTypeId Actors::drink_getFluidType(ActorIndex index) const
{
	return m_mustDrink[index]->getFluidType();
}
Percent Actors::drink_getPercentDead(ActorIndex index) const
{
	return m_mustDrink[index]->getPercentDead();
}
Step Actors::drink_getStepsTillDead(ActorIndex index) const
{
	return m_mustDrink[index]->getStepsTillDead();
}
bool Actors::drink_hasThristEvent(ActorIndex index) const
{
	return m_mustDrink[index]->thirstEventExists();
}
