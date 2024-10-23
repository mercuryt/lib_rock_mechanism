#include "actors.h"
#include "../temperature.h"
void Actors::temperature_onChange(const ActorIndex& index)
{
	m_needsSafeTemperature[index]->onChange(m_area);
}
bool Actors::temperature_isSafe(const ActorIndex& index, const Temperature& temperature) const
{
	return m_needsSafeTemperature[index]->isSafe(m_area, temperature);
}
bool Actors::temperature_isSafeAtCurrentLocation(const ActorIndex& index) const
{
	return m_needsSafeTemperature[index]->isSafeAtCurrentLocation(m_area);
}
