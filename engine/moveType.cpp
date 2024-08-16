#include "moveType.h"
MoveTypeId MoveType::byName(std::string name)
{ 
	auto found = moveTypeData.m_name.find(name);
	assert(found != moveTypeData.m_name.end());
	return MoveTypeId::create(found - moveTypeData.m_name.begin());
}
void MoveType::create(MoveTypeParamaters& p)
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
}
std::string MoveType::getName(MoveTypeId id) { return moveTypeData.m_name[id]; }
bool MoveType::getWalk(MoveTypeId id) { return moveTypeData.m_walk[id]; }
uint8_t MoveType::getClimb(MoveTypeId id) { return moveTypeData.m_climb[id]; }
bool MoveType::getJumpDown(MoveTypeId id) { return moveTypeData.m_jumpDown[id]; }
bool MoveType::getFly(MoveTypeId id) { return moveTypeData.m_fly[id]; }
bool MoveType::getBreathless(MoveTypeId id) { return moveTypeData.m_breathless[id]; }
bool MoveType::getOnlyBreathsFluids(MoveTypeId id) { return moveTypeData.m_onlyBreathsFluids[id]; }
FluidTypeMap<CollisionVolume>& MoveType::getSwim(MoveTypeId id) { return moveTypeData.m_swim[id]; }
FluidTypeSet& MoveType::getBreathableFluids(MoveTypeId id) { return moveTypeData.m_breathableFluids[id]; }
