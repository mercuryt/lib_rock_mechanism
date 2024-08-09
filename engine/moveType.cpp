#include "moveType.h"
MoveTypeId MoveType::byName(std::string name)
{ 
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return MoveTypeId::create(found - data.m_name.begin());
}
void MoveType::create(MoveTypeParamaters& p)
{
	data.m_name.add(p.name);
	data.m_walk.add(p.walk);
	data.m_climb.add(p.climb);
	data.m_jumpDown.add(p.jumpDown);
	data.m_fly.add(p.fly);
	data.m_breathless.add(p.breathless);
	data.m_onlyBreathsFluids.add(p.onlyBreathsFluids);
	data.m_swim.add(p.swim);
	data.m_breathableFluids.add(p.breathableFluids);
}
