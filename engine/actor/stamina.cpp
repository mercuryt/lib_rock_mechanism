#include "stamina.h"
#include "config.h"
#include <algorithm>
#include <cassert>
void ActorHasStamina::recover()
{
	// TODO: modifier based on physiology.
	const uint32_t& quantity = Config::staminaPointsPerRestPeriod;
	m_stamina = std::min(getMax(), m_stamina + quantity);
}
void ActorHasStamina::spend(uint32_t stamina)
{
	assert(m_stamina >= stamina);
	m_stamina -= stamina;
}
void ActorHasStamina::setFull()
{
	m_stamina = getMax();
}
uint32_t ActorHasStamina::getMax() const { return Config::maxStaminaPointsBase;}
bool ActorHasStamina::hasAtLeast(uint32_t stamina) const { return m_stamina >= stamina; }
bool ActorHasStamina::isFull() const { return m_stamina == getMax(); }