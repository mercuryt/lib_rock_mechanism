#include "actors.h"
#include "../drink.h"
void Actors::drink_do(const ActorIndex& index, const CollisionVolume& volume)
{
	m_mustDrink[index]->drink(m_area, volume);
}
void Actors::drink_setNeedsFluid(const ActorIndex& index)
{
	m_mustDrink[index]->unschedule();
	m_mustDrink[index]->setNeedsFluid(m_area);
}
void Actors::drink_setNeverThirsty(const ActorIndex& index)
{
	m_mustDrink[index]->unschedule();
}
CollisionVolume Actors::drink_getVolumeOfFluidRequested(const ActorIndex& index) const
{
	return m_mustDrink[index]->getVolumeFluidRequested();
}
bool Actors::drink_isThirsty(const ActorIndex& index) const
{
	return m_mustDrink[index]->needsFluid();
}
FluidTypeId Actors::drink_getFluidType(const ActorIndex& index) const
{
	return m_mustDrink[index]->getFluidType();
}
Percent Actors::drink_getPercentDead(const ActorIndex& index) const
{
	return m_mustDrink[index]->getPercentDead();
}
Step Actors::drink_getStepsTillDead(const ActorIndex& index) const
{
	return m_mustDrink[index]->getStepsTillDead();
}
bool Actors::drink_hasThristEvent(const ActorIndex& index) const
{
	return m_mustDrink[index]->thirstEventExists();
}
