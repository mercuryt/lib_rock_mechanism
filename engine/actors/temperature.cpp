#include "actors.h"
#include "../safeTemperature.h"
#include "../definitions/animalSpecies.h"
void Actors::temperature_onChange(ActorIndex index)
{
	m_needsSafeTemperature[index]->onChange(m_area);
}
bool Actors::temperature_isSafe(ActorIndex index, const Temperature temperature) const
{
	return m_needsSafeTemperature[index]->isSafe(m_area, temperature);
}
bool Actors::temperature_isSafeAtCurrentLocation(ActorIndex index) const
{
	return m_needsSafeTemperature[index]->isSafeAtCurrentLocation(m_area);
}
Temperature Actors::temperature_getMaxSafe(ActorIndex index) const
{
	return AnimalSpecies::getMaximumSafeTemperature(m_species[index]);
}
Temperature Actors::temperature_getMinSafe(ActorIndex index) const
{
	return AnimalSpecies::getMinimumSafeTemperature(m_species[index]);
}
