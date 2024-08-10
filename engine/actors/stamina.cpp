#include "actors.h"
#include "config.h"
#include <algorithm>
#include <cassert>
void Actors::stamina_recover(ActorIndex index)
{
	// TODO: modifier based on physiology.
	m_stamina[index] = std::min(stamina_getMax(index), m_stamina[index] + Config::staminaPointsPerRestPeriod);
}
void Actors::stamina_spend(ActorIndex index, Stamina stamina)
{
	assert(m_stamina[index] >= stamina);
	m_stamina[index] -= stamina;
}
void Actors::stamina_setFull(ActorIndex index)
{
	m_stamina[index] = stamina_getMax(index);
}
Stamina Actors::stamina_getMax(ActorIndex) const { return Config::maxStaminaPointsBase;}
bool Actors::stamina_hasAtLeast(ActorIndex index, Stamina stamina) const { return m_stamina[index] >= stamina; }
bool Actors::stamina_isFull(ActorIndex index) const { return m_stamina[index] == stamina_getMax(index); }
