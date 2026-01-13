#include "visionCuboid.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
#include "../space/space.h"
#include "../actors/actors.h"

VisionCuboidSetSIMD::VisionCuboidSetSIMD(int capacity) :
	m_cuboidSet(capacity),
	m_indices(capacity)
{ }
void VisionCuboidSetSIMD::insert(const VisionCuboidId& index, const Cuboid& cuboid)
{
	m_cuboidSet.insert(cuboid);
	if(m_cuboidSet.capacity() > m_indices.size())
		m_indices.conservativeResize(m_cuboidSet.capacity());
	m_indices[m_cuboidSet.size()] = index.get();
}
void VisionCuboidSetSIMD::maybeInsert(const VisionCuboidId& index, const Cuboid& cuboid)
{
	if(!contains(index))
		insert(index, cuboid);
}
void VisionCuboidSetSIMD::clear() { m_indices.fill(VisionCuboidId::null().get()); m_cuboidSet.clear(); }
bool VisionCuboidSetSIMD::intersects(const Cuboid& cuboid) const { return m_cuboidSet.intersects(cuboid); }
bool VisionCuboidSetSIMD::contains(const VisionCuboidId& index) const { return (m_indices == index.get()).any(); }
bool VisionCuboidSetSIMD::contains(const VisionCuboidIdWidth& index) const { return (m_indices == index).any(); }

AreaHasVisionCuboids::AreaHasVisionCuboids(Area& area) : m_area(area) { }
void AreaHasVisionCuboids::initalize()
{
	const Space& space = m_area.getSpace();
	insertOrMerge(space.boundry());
}
void AreaHasVisionCuboids::pointIsTransparent(const Point3D& point)
{
	assert(maybeGetVisionCuboidIndexForPoint(point).empty());
	maybeAdd({point, point});
}
void AreaHasVisionCuboids::pointIsOpaque(const Point3D& point)
{
	Cuboid cuboid(point, point);
	maybeRemove(cuboid);
}
void AreaHasVisionCuboids::pointFloorIsTransparent(const Point3D& point)
{
	mergeBelow({point, point});
}
void AreaHasVisionCuboids::pointFloorIsOpaque(const Point3D& point)
{
	sliceBelow({point, point});
}
void AreaHasVisionCuboids::cuboidIsTransparent(const Cuboid& cuboid)
{
	maybeAdd(cuboid);
}
void AreaHasVisionCuboids::cuboidIsOpaque(const Cuboid& cuboid)
{
	maybeRemove(cuboid);
}
bool AreaHasVisionCuboids::canSeeInto(const Cuboid& a, const Cuboid& b) const
{
	assert(a != b);
	assert(a.m_high != b.m_high);
	assert(a.m_low != b.m_low);
	Space& space = m_area.getSpace();
	// Get a cuboid representing a face of m_cuboid.
	Facing6 facing = a.getFacingTwordsOtherCuboid(b);

	const Cuboid face = a.getFace(facing);
	// Verify that the whole face can be seen through from the direction of m_cuboid.
	for(const Point3D& point : face)
	{
		const auto adjacent = Point3D::create(point.moveInDirection(facing, Distance::create(1)));
		if(!space.canSeeIntoFromAlways(adjacent, point))
			return false;
	};
	return true;
}
void AreaHasVisionCuboids::onKeySetForPoint(const VisionCuboidId& newIndex, const Point3D& point)
{
	Space& space = m_area.getSpace();
	Actors& actors = m_area.getActors();
	for(const ActorIndex& actor : space.actor_getAll(point))
		m_area.m_visionRequests.maybeUpdateCuboid(actors.getReference(actor), newIndex);
	m_area.m_octTree.updateVisionCuboid(point, newIndex);
}
SmallSet<int> AreaHasVisionCuboids::getMergeCandidates(const Cuboid& cuboid) const
{
	// TODO: SmallSetLimited<6>.
	SmallSet<int> output;
	output.reserve(6);
	SmallSet<Point3D> candidatePoints;
	candidatePoints.reserve(6);
	Cuboid boundry = m_area.getSpace().boundry();
	if(cuboid.m_high.z() < boundry.m_high.z())
		candidatePoints.insert(cuboid.m_high.above());
	if(cuboid.m_high.y() < boundry.m_high.y())
		candidatePoints.insert(cuboid.m_high.south());
	if(cuboid.m_high.x() < boundry.m_high.x())
		candidatePoints.insert(cuboid.m_high.east());
	if(cuboid.m_low.z() != 0)
		candidatePoints.insert(cuboid.m_low.below());
	if(cuboid.m_low.y() != 0)
		candidatePoints.insert(cuboid.m_low.north());
	if(cuboid.m_low.x() != 0)
		candidatePoints.insert(cuboid.m_low.west());
	for(const Point3D& point : candidatePoints)
	{
		VisionCuboidId visionCuboidIndex = m_pointLookup.queryGetOne(point);
		if(visionCuboidIndex.exists())
		{
			int index = getIndexForVisionCuboidId(visionCuboidIndex);
			output.insert(index);
		}
	}
	return output;
};
void AreaHasVisionCuboids::insertOrMerge(const Cuboid& cuboid)
{
	validate();
	for(const int& index : getMergeCandidates(cuboid))
	{
		const Cuboid& otherCuboid = getCuboidByIndex(index);
		assert(!cuboid.intersects(otherCuboid));
		if(otherCuboid.isTouching(cuboid) && otherCuboid.canMerge(cuboid) && canSeeInto(cuboid, otherCuboid))
		{
			mergeInternal(cuboid, index);
			return;
		}
	}
	VisionCuboidId key = m_nextKey;
	++m_nextKey;
	// Allocate.
	m_cuboids.insert(cuboid);
	m_keys.push_back(key);
	m_adjacent.emplace_back();
	m_pointLookup.maybeInsert(cuboid, key);
	for(const Point3D& point : cuboid)
		onKeySetForPoint(key, point);
	// Adjacent.
	CuboidMap<VisionCuboidId>& adjacent = m_adjacent.back();
	for(const auto& [point, facing] : cuboid.getSurfaceView())
	{
		const Offset3D& outside = point.moveInDirection(facing, Distance::create(1));
		VisionCuboidId outsideKey = m_pointLookup.queryGetOne(Point3D::create(outside));
		if(outsideKey.exists() && !adjacent.contains(outsideKey))
		{
			adjacent.insert(outsideKey, getCuboidByVisionCuboidId(outsideKey));
			const int outsideIndex = getIndexForVisionCuboidId(outsideKey);
			m_adjacent[outsideIndex].insert(key, cuboid);
			validate();
		}
	}
	validate();
}
void AreaHasVisionCuboids::destroy(const int& index)
{
	validate();
	const Cuboid cuboid = m_cuboids[index];
	const VisionCuboidId& key = m_keys[index];
	m_pointLookup.maybeRemove(cuboid);
	// Adjacent.
	for(const auto& [otherKey, otherCuboid] : m_adjacent[index])
	{
		const int& otherIndex = getIndexForVisionCuboidId(otherKey);
		assert(m_adjacent[otherIndex].contains(key));
		assert(m_adjacent[otherIndex][key] == m_cuboids[index]);
		assert(m_adjacent[index][otherKey] == otherCuboid);
		m_adjacent[otherIndex].erase(key);
	}
	if(index != (int)m_keys.size() - 1)
	{
		m_adjacent[index] = m_adjacent.back();
		m_keys[index] = m_keys.back();
	}
	m_adjacent.pop_back();
	m_keys.pop_back();
	m_cuboids.eraseIndex(index);
	validate();
}
void AreaHasVisionCuboids::maybeRemove(const Cuboid& cuboid)
{
	//TODO: partition instead of toSplit.
	SmallMap<VisionCuboidId, Cuboid> toSplit;
	for(const Point3D& point : cuboid)
	{
		VisionCuboidId existing = m_pointLookup.queryGetOne(point);
		if(existing.exists())
			toSplit.maybeInsert(existing, getCuboidByVisionCuboidId(existing));
	}
	for(const auto& pair : toSplit)
		destroy(getIndexForVisionCuboidId(pair.first));
	for(const auto& pair : toSplit)
		for(const Cuboid& splitResult : pair.second.getChildrenWhenSplitByCuboid(cuboid))
			insertOrMerge(splitResult);
}
void AreaHasVisionCuboids::maybeRemove(const CuboidSet& cuboids)
{
	for(const Cuboid& cuboid : cuboids)
		maybeRemove(cuboid);
}
void AreaHasVisionCuboids::sliceBelow(const Cuboid& cuboid)
{
	assert(cuboid.dimensionForFacing(Facing6::Above) == 1);
	// Record key and cuboid instead of index because index may change as keys and cuboids are destroyed.
	SmallSet<std::pair<VisionCuboidId, Cuboid>> toSplit;
	int end = m_keys.size();
	for(int i = 0; i != end; ++i)
		if(cuboid.intersects(m_cuboids[i]))
			toSplit.emplace(m_keys[i], m_cuboids[i]);
	for(const auto& [key, splitCuboid] : toSplit)
	{
		int index = getIndexForVisionCuboidId(key);
		destroy(index);
	}
	for(const auto& pair : toSplit)
	{
		auto children = pair.second.getChildrenWhenSplitBelowCuboid(cuboid);
		if(children.first.exists())
			insertOrMerge(children.first);
		if(children.second.exists())
			insertOrMerge(children.second);
	}
}
void AreaHasVisionCuboids::mergeBelow(const Cuboid& cuboid)
{
	SmallSet<std::pair<VisionCuboidId, Cuboid>> toMerge;
	for(int i = 0; i < (int)m_keys.size(); ++i)
		if(cuboid.intersects(m_cuboids[i]))
			toMerge.emplace(m_keys[i], m_cuboids[i]);
	for(const auto& [key, mergeCuboid] : toMerge)
	{
		int index = getIndexForVisionCuboidId(key);
		destroy(index);
	}
	for(const auto& [key, mergeCuboid] : toMerge)
		// create will clear the current cuboid, create a new one in the same place and attempt to merge it as normal.
		// TODO: Identify cuboids which will be replicated.
		insertOrMerge(mergeCuboid);
}
int AreaHasVisionCuboids::getIndexForVisionCuboidId(const VisionCuboidId& key) const
{
	auto found = std::ranges::find(m_keys, key);
	assert(found != m_keys.end());
	return std::distance(m_keys.begin(), found);
}
Cuboid AreaHasVisionCuboids::getCuboidByVisionCuboidId(const VisionCuboidId& key) const
{
	int index = getIndexForVisionCuboidId(key);
	return getCuboidByIndex(index);
}
void AreaHasVisionCuboids::validate() const
{
	for(int i = 0; i < (int)m_keys.size(); ++i)
	{
		[[maybe_unused]] const VisionCuboidId& key = m_keys[i];
		[[maybe_unused]] const Cuboid& cuboid = m_cuboids[i];
		for(const auto& [adjacentKey, adjacentCuboid] : m_adjacent[i])
		{
			int adjacentI = getIndexForVisionCuboidId(adjacentKey);
			CuboidMap<VisionCuboidId> adjacentAdjacent = m_adjacent[adjacentI];
			assert(adjacentAdjacent.contains(key));
			assert(adjacentAdjacent[key] == cuboid);
			assert(m_adjacent[i].contains(adjacentKey));
			assert(m_adjacent[i][adjacentKey] == adjacentCuboid);
		}
	}
}