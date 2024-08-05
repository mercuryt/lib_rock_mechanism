#include "moveType.h"
MoveTypeId MoveType::byName(std::string name)
{ 
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return MoveTypeId::create(found - data.m_name.begin());
}
void MoveType::create(MoveTypeParamaters& p)
{
	m_name.add(p.name);
	m_walk.add(p.walk);
	m_climb.add(p.climb);
	m_jumpDown.add(p.jumpDown);
	m_fly.add(p.fly);
	m_breathless.add(p.breathless);
	m_onlyBreathsFluids.add(p.onlyBreathsFluids);
	m_swim.add(p.swim);
	m_breathableFluids.add(p.breathableFluids);
}
