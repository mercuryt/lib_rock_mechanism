#include "actors.h"
#include "../drink.h"
void Actors::drink_do(ActorIndex index, CollisionVolume volume)
{
	m_mustDrink[index]->drink(m_area, volume);
}
void Actors::drink_setNeedsFluid(ActorIndex index)
{
	m_mustDrink[index]->setNeedsFluid(m_area);
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
bool Actors::drink_hasThristEvent(ActorIndex index) const
{
	return m_mustDrink[index]->thirstEventExists();
}
