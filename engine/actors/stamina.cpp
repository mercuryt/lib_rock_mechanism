#include "actors.h"
#include "config.h"
#include <algorithm>
#include <cassert>
void Actors::stamina_recover(const ActorIndex& index)
{
	// TODO: modifier based on physiology.
	m_stamina[index] = std::min(stamina_getMax(index), m_stamina[index] + Config::staminaPointsPerRestPeriod);
}
void Actors::stamina_spend(const ActorIndex& index, const Stamina& stamina)
{
	assert(m_stamina[index] >= stamina);
	m_stamina[index] -= stamina;
}
void Actors::stamina_setFull(const ActorIndex& index)
{
	m_stamina[index] = stamina_getMax(index);
}
Stamina Actors::stamina_getMax(const ActorIndex&) const { return Config::maxStaminaPointsBase;}
bool Actors::stamina_hasAtLeast(const ActorIndex& index, const Stamina& stamina) const { return m_stamina[index] >= stamina; }
bool Actors::stamina_isFull(const ActorIndex& index) const { return m_stamina[index] == stamina_getMax(index); }
