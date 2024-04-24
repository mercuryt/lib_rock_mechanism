#pragma once

#include <cstdint>

class Actor;

class ActorHasStamina final
{
	Actor& m_actor;
	uint32_t m_stamina;
public:
	ActorHasStamina(Actor& a) : m_actor(a), m_stamina(getMax()) { }
	ActorHasStamina(Actor& a, uint32_t stamina) : m_actor(a), m_stamina(stamina) { }
	void recover();
	void spend(uint32_t stamina);
	void setFull();
	bool hasAtLeast(uint32_t stamina) const;
	bool isFull() const;
	uint32_t getMax() const;
	uint32_t get() const { return m_stamina; }
};
