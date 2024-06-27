#include "actors.h"
#include "../drink.h"
void Actors::drink_do(ActorIndex index, CollisionVolume volume)
{
	m_mustDrink.at(index)->drink(m_area, volume);
}
void Actors::drink_setNeedsFluid(ActorIndex index)
{
	m_mustDrink.at(index)->setNeedsFluid(m_area);
}
CollisionVolume Actors::drink_getVolumeOfFluidRequested(ActorIndex index)
{
	return m_mustDrink.at(index)->getVolumeFluidRequested();
}
bool Actors::drink_needsFluid(ActorIndex index)
{
	return m_mustDrink.at(index)->needsFluid();
}
const FluidType& Actors::drink_getFluidType(ActorIndex index)
{
	return m_mustDrink.at(index)->getFluidType();
}
