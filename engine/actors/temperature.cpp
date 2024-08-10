#include "actors.h"
#include "../temperature.h"
void Actors::temperature_onChange(ActorIndex index)
{
	m_needsSafeTemperature[index]->onChange(m_area);
}
bool Actors::temperature_isSafe(ActorIndex index, Temperature temperature) const
{
	return m_needsSafeTemperature[index]->isSafe(m_area, temperature);
}
bool Actors::temperature_isSafeAtCurrentLocation(ActorIndex index) const
{
	return m_needsSafeTemperature[index]->isSafeAtCurrentLocation(m_area);
}
