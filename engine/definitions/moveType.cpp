#include "definitions/moveType.h"
MoveTypeId MoveType::byName(const std::string& name)
{
	auto found = g_moveTypeData.m_name.find(name);
	assert(found != g_moveTypeData.m_name.end());
	return MoveTypeId::create(found - g_moveTypeData.m_name.begin());
}
void MoveType::create(const MoveTypeParamaters& p)
{
	g_moveTypeData.m_name.add(p.name);
	g_moveTypeData.m_surface.add(p.surface);
	g_moveTypeData.m_stairs.add(p.stairs);
	g_moveTypeData.m_climb.add(p.climb);
	g_moveTypeData.m_jumpDown.add(p.jumpDown);
	g_moveTypeData.m_fly.add(p.fly);
	g_moveTypeData.m_breathless.add(p.breathless);
	g_moveTypeData.m_onlyBreathsFluids.add(p.onlyBreathsFluids);
	g_moveTypeData.m_swim.add(p.swim);
	g_moveTypeData.m_breathableFluids.add(p.breathableFluids);
	g_moveTypeData.m_floating.add(p.floating);
	g_moveTypeData.m_paramaters.add(p);
}
std::string MoveType::getName(const MoveTypeId& id) { return g_moveTypeData.m_name[id]; }
bool MoveType::getSurface(const MoveTypeId& id) { return g_moveTypeData.m_surface[id]; }
bool MoveType::getStairs(const MoveTypeId& id) { return g_moveTypeData.m_stairs[id]; }
uint8_t MoveType::getClimb(const MoveTypeId& id) { return g_moveTypeData.m_climb[id]; }
bool MoveType::getJumpDown(const MoveTypeId& id) { return g_moveTypeData.m_jumpDown[id]; }
bool MoveType::getFly(const MoveTypeId& id) { return g_moveTypeData.m_fly[id]; }
bool MoveType::getBreathless(const MoveTypeId& id) { return g_moveTypeData.m_breathless[id]; }
bool MoveType::getOnlyBreathsFluids(const MoveTypeId& id) { return g_moveTypeData.m_onlyBreathsFluids[id]; }
bool MoveType::getFloating(const MoveTypeId& id) { return g_moveTypeData.m_floating[id]; }
SmallMap<FluidTypeId, CollisionVolume>& MoveType::getSwim(const MoveTypeId& id) { return g_moveTypeData.m_swim[id]; }
SmallSet<FluidTypeId>& MoveType::getBreathableFluids(const MoveTypeId& id) { return g_moveTypeData.m_breathableFluids[id]; }