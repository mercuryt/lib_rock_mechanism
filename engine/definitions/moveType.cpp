#include "definitions/moveType.h"
#include "../fluidType.h"
bool MoveTypeParamaters::operator==(const MoveTypeParamaters& other) const
{
	if(!(
		name == other.name &&
		surface == other.surface &&
		stairs == other.stairs &&
		climb == other.climb &&
		jumpDown == other.jumpDown &&
		fly == other.fly &&
		breathless == other.breathless &&
		onlyBreathsFluids == other.onlyBreathsFluids &&
		floating == other.floating &&
		swim.size() == other.swim.size() &&
		breathableFluids.size() == other.breathableFluids.size()
	))
		return false;
	for(auto [k,v] : swim)
		if(other.swim[k] != v)
			return false;
	if(!breathableFluids.containsAll(other.breathableFluids))
		return false;
	return true;
}
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
MoveTypeId MoveType::getOrCreate(const MoveTypeParamaters& p)
{
	auto found = std::ranges::find(g_moveTypeData.m_paramaters.toVector(), p);
	if(found != g_moveTypeData.m_paramaters.end())
		return MoveTypeId::create(found - g_moveTypeData.m_paramaters.begin());
	create(p);
	return MoveTypeId::create(g_moveTypeData.m_name.size() - 1);
}
std::string MoveType::getName(const MoveTypeId id) { return g_moveTypeData.m_name[id]; }
bool MoveType::getSurface(const MoveTypeId id) { return g_moveTypeData.m_surface[id]; }
bool MoveType::getStairs(const MoveTypeId id) { return g_moveTypeData.m_stairs[id]; }
int MoveType::getClimb(const MoveTypeId id) { return g_moveTypeData.m_climb[id]; }
bool MoveType::getJumpDown(const MoveTypeId id) { return g_moveTypeData.m_jumpDown[id]; }
bool MoveType::getFly(const MoveTypeId id) { return g_moveTypeData.m_fly[id]; }
bool MoveType::getBreathless(const MoveTypeId id) { return g_moveTypeData.m_breathless[id]; }
bool MoveType::getOnlyBreathsFluids(const MoveTypeId id) { return g_moveTypeData.m_onlyBreathsFluids[id]; }
std::pair<FluidTypeId, Distance> MoveType::getFloating(const MoveTypeId id) { return g_moveTypeData.m_floating[id]; }
SmallMap<FluidTypeId, CollisionVolume>& MoveType::getSwim(const MoveTypeId id) { return g_moveTypeData.m_swim[id]; }
SmallSet<FluidTypeId>& MoveType::getBreathableFluids(const MoveTypeId id) { return g_moveTypeData.m_breathableFluids[id]; }
MoveTypeId MoveType::getOrCreateForFloat(FluidTypeId fluidType, Distance depth)
{
	const auto end = g_moveTypeData.m_name.size();
	for(MoveTypeId id{0}; id != end; ++id)
	{
		auto [otherFluidType, otherDepth] = getFloating(id);
		if(otherFluidType == fluidType && otherDepth == depth)
			return id;
	}
	// No existing move type found, create new one.
	create({
		.name = "float in " + FluidType::getName(fluidType) + " at depth " + depth.toS(),
		.surface = false,
		.stairs = false,
		.climb = 0,
		.jumpDown = false,
		.fly = false,
		.breathless = false,
		.onlyBreathsFluids = false,
		.floating = {fluidType, depth},
	});
	return MoveTypeId::create(g_moveTypeData.m_name.size() - 1);
}
MoveTypeId MoveType::reduce(const SmallSet<MoveTypeId>& moveTypes)
{
	// Make a copy.
	MoveTypeParamaters params = g_moveTypeData.m_paramaters[moveTypes.front()];
	for(int i{1}; i < moveTypes.size(); ++i)
	{
		MoveTypeId other = moveTypes[i];
		params.surface = std::min(params.surface, getSurface(other));
		params.climb = std::min(params.climb, getClimb(other));
		params.jumpDown = std::min(params.jumpDown, getJumpDown(other));
		if(getFly(other))
		{
			params.fly = false;
			if(getSurface(other))
				params.surface = true;
		}
		params.breathless = std::min(params.breathless, getBreathless(other));
		// Only breaths fluids is more restricive so max is used here rather then min.
		params.onlyBreathsFluids = std::max(params.onlyBreathsFluids, getOnlyBreathsFluids(other));
		if(params.floating.first.exists())
		{
			assert(getFloating(other).first == params.floating.first);
			params.floating.second = std::max(params.floating.second, getFloating(other).second);
		}
		auto otherSwim = getSwim(other);
		auto copy = params.swim;
		for(auto [fluidType, depth] : params.swim)
		{
			auto found = otherSwim.find(fluidType);
			if(found == otherSwim.end())
				copy.erase(fluidType);
			copy[fluidType] = std::max(params.swim[fluidType], found->second);
		}
		params.swim = copy;
		auto otherBreathableFluids = getBreathableFluids(other);
		auto copy2 = params.breathableFluids;
		for(auto fluidType : params.breathableFluids)
			if(!otherBreathableFluids.contains(fluidType))
				copy2.erase(fluidType);
		params.breathableFluids = copy2;
	}
	return getOrCreate(params);
}