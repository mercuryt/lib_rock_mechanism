#include "moveType.h"
MoveTypeId MoveType::byName(const std::string& name)
{
	auto found = moveTypeData.m_name.find(name);
	assert(found != moveTypeData.m_name.end());
	return MoveTypeId::create(found - moveTypeData.m_name.begin());
}
void MoveType::create(const MoveTypeParamaters& p)
{
	moveTypeData.m_name.add(p.name);
	moveTypeData.m_walk.add(p.walk);
	moveTypeData.m_climb.add(p.climb);
	moveTypeData.m_jumpDown.add(p.jumpDown);
	moveTypeData.m_fly.add(p.fly);
	moveTypeData.m_breathless.add(p.breathless);
	moveTypeData.m_onlyBreathsFluids.add(p.onlyBreathsFluids);
	moveTypeData.m_swim.add(p.swim);
	moveTypeData.m_breathableFluids.add(p.breathableFluids);
	moveTypeData.m_floating.add(p.floating);
	moveTypeData.m_paramaters.add(p);
}
std::string MoveType::getName(const MoveTypeId& id) { return moveTypeData.m_name[id]; }
bool MoveType::getWalk(const MoveTypeId& id) { return moveTypeData.m_walk[id]; }
uint8_t MoveType::getClimb(const MoveTypeId& id) { return moveTypeData.m_climb[id]; }
bool MoveType::getJumpDown(const MoveTypeId& id) { return moveTypeData.m_jumpDown[id]; }
bool MoveType::getFly(const MoveTypeId& id) { return moveTypeData.m_fly[id]; }
bool MoveType::getBreathless(const MoveTypeId& id) { return moveTypeData.m_breathless[id]; }
bool MoveType::getOnlyBreathsFluids(const MoveTypeId& id) { return moveTypeData.m_onlyBreathsFluids[id]; }
bool MoveType::getFloating(const MoveTypeId& id) { return moveTypeData.m_floating[id]; }
SmallMap<FluidTypeId, CollisionVolume>& MoveType::getSwim(const MoveTypeId& id) { return moveTypeData.m_swim[id]; }
SmallSet<FluidTypeId>& MoveType::getBreathableFluids(const MoveTypeId& id) { return moveTypeData.m_breathableFluids[id]; }