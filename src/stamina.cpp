#include "stamina.h"
#include "actor.h"

void RestEvent::execute()
{
	m_objective.m_actor.m_stamina.recover();
	m_objective.m_actor.m_hasObjectives.objectiveComplete(m_objective);
}
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
uint32_t ActorHasStamina::getMax() const { return Config::staminaPointsPerRestPeriod;}
bool ActorHasStamina::hasAtLeast(uint32_t stamina) const { return m_stamina >= stamina; }
bool ActorHasStamina::isFull() const { return m_stamina == getMax(); }
