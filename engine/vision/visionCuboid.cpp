#include "vision/visionCuboid.h"
#include "area/area.h"
#include "geometry/sphere.h"
#include "numericTypes/types.h"
#include "space/space.h"
#include "actors/actors.h"
#include "partitionNotify.h"

VisionCuboidSetSIMD::VisionCuboidSetSIMD(uint capacity) :
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
	create(space.getAll());
}
void AreaHasVisionCuboids::pointIsTransparent(const Point3D& point)
{
	assert(maybeGetVisionCuboidIndexForPoint(point).empty());
	add({point, point});
}
void AreaHasVisionCuboids::pointIsOpaque(const Point3D& point)
{
	Cuboid cuboid(point, point);
	remove(cuboid);
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
	add(cuboid);
}
void AreaHasVisionCuboids::cuboidIsOpaque(const Cuboid& cuboid)
{
	remove(cuboid);
}
bool AreaHasVisionCuboids::canSeeInto(const Cuboid& a, const Cuboid& b) const
{
	assert(a != b);
	assert(a.m_highest != b.m_highest);
	assert(a.m_lowest != b.m_lowest);
	Space& space = m_area.getSpace();
	// Get a cuboid representing a face of m_cuboid.
	Facing6 facing = a.getFacingTwordsOtherCuboid(b);

	const Cuboid face = a.getFace(facing);
	// Verify that the whole face can be seen through from the direction of m_cuboid.
	for(const Point3D& point : face)
	{
		assert(face.contains(point));
		assert(space.getAll().contains(point.moveInDirection(facing, Distance::create(1))));
		if(!space.canSeeIntoFromAlways(point.moveInDirection(facing, Distance::create(1)), point))
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
SmallSet<uint> AreaHasVisionCuboids::getMergeCandidates(const Cuboid& cuboid) const
{
	Space& space = m_area.getSpace();
	SmallSet<uint> output;
	std::array<Point3D, 6> candidatePoints = {
		cuboid.m_highest.above(),
		cuboid.m_highest.south(),
		cuboid.m_highest.east(),
		cuboid.m_lowest.below(),
		cuboid.m_lowest.north(),
		cuboid.m_lowest.west(),
	};
	for(const Point3D& point : candidatePoints)
		if(point.exists())
		{
			VisionCuboidId visionCuboidIndex = m_pointLookup.queryGetOne(point);
			if(visionCuboidIndex.exists())
			{
				uint index = getIndexForVisionCuboidId(visionCuboidIndex);
				output.insert(index);
			}
		}
	return output;
};
void AreaHasVisionCuboids::create(const Cuboid& cuboid)
{
	validate();
	for(const uint& index : getMergeCandidates(cuboid))
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
	for(const Point3D& point : cuboid)
	{
		onKeySetForPoint(key, point);
		m_pointLookup.queryGetOne(point) = key;
	}
	// Adjacent.
	const Space& space = m_area.getSpace();
	CuboidMap<VisionCuboidId>& adjacent = m_adjacent.back();
	for(const auto& [point, facing] : cuboid.getSurfaceView())
	{
		const Point3D& outside = point.moveInDirection(facing, Distance::create(1));
		VisionCuboidId outsideKey = m_pointLookup.queryGetOne(outside);
		if(outsideKey.exists() && !adjacent.contains(outsideKey))
		{
			adjacent.insert(outsideKey, getCuboidByVisionCuboidId(outsideKey));
			const uint outsideIndex = getIndexForVisionCuboidId(outsideKey);
			m_adjacent[outsideIndex].insert(key, cuboid);
			validate();
		}
	}
	validate();
}
void AreaHasVisionCuboids::destroy(const uint& index)
{
	validate();
	const Cuboid cuboid = m_cuboids[index];
	const VisionCuboidId& key = m_keys[index];
	for(const Point3D& point : cuboid)
		m_pointLookup.queryGetOne(point).clear();
	// Adjacent.
	for(const auto& [otherKey, otherCuboid] : m_adjacent[index])
	{
		const uint& otherIndex = getIndexForVisionCuboidId(otherKey);
		assert(m_adjacent[otherIndex].contains(key));
		assert(m_adjacent[otherIndex][key] == m_cuboids[index]);
		assert(m_adjacent[index][otherKey] == otherCuboid);
		m_adjacent[otherIndex].erase(key);
	}
	if(index != m_keys.size() - 1)
	{
		m_adjacent[index] = m_adjacent.back();
		m_keys[index] = m_keys.back();
	}
	m_adjacent.pop_back();
	m_keys.pop_back();
	m_cuboids.eraseIndex(index);
	validate();
}
void AreaHasVisionCuboids::remove(const Cuboid& cuboid)
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
			create(splitResult);
}
void AreaHasVisionCuboids::remove(const SmallSet<Point3D>& toRemove)
{
	Space& space = m_area.getSpace();
	SmallSet<Point3D> points;
	for(const Point3D& point : toRemove)
		points.insert(point);
	auto cuboids = CuboidSet::create(points);
	for(const Cuboid& cuboid : cuboids)
		remove(cuboid);
}
void AreaHasVisionCuboids::remove(const SmallSet<Point3D>& points)
{
	auto cuboids = CuboidSet::create(points);
	for(const Cuboid& cuboid : cuboids)
		remove(cuboid);
}
void AreaHasVisionCuboids::sliceBelow(const Cuboid& cuboid)
{
	assert(cuboid.dimensionForFacing(Facing6::Above) == 1);
	// Record key and cuboid instead of index because index may change as keys and cuboids are destroyed.
	SmallSet<std::pair<VisionCuboidId, Cuboid>> toSplit;
	for(uint i = 0; i < m_keys.size(); ++i)
		if(cuboid.intersects(m_cuboids[i]))
			toSplit.emplace(m_keys[i], m_cuboids[i]);
	for(const auto& [key, cuboid] : toSplit)
	{
		uint index = getIndexForVisionCuboidId(key);
		destroy(index);
	}
	for(const auto& pair : toSplit)
	{
		auto children = pair.second.getChildrenWhenSplitBelowCuboid(cuboid);
		if(children.first.exists())
			create(children.first);
		if(children.second.exists())
			create(children.second);
	}
}
void AreaHasVisionCuboids::mergeBelow(const Cuboid& cuboid)
{
	SmallSet<std::pair<VisionCuboidId, Cuboid>> toMerge;
	for(uint i = 0; i < m_keys.size(); ++i)
		if(cuboid.intersects(m_cuboids[i]))
			toMerge.emplace(m_keys[i], m_cuboids[i]);
	for(const auto& [key, cuboid] : toMerge)
	{
		uint index = getIndexForVisionCuboidId(key);
		destroy(index);
	}
	for(const auto& [key, cuboid] : toMerge)
		// create will clear the current cuboid, create a new one in the same place and attempt to merge it as normal.
		// TODO: Identify cuboids which will be replicated.
		create(cuboid);
}
uint AreaHasVisionCuboids::getIndexForVisionCuboidId(const VisionCuboidId& key) const
{
	auto found = std::ranges::find(m_keys, key);
	assert(found != m_keys.end());
	return std::distance(m_keys.begin(), found);
}
Cuboid AreaHasVisionCuboids::getCuboidByVisionCuboidId(const VisionCuboidId& key) const
{
	uint index = getIndexForVisionCuboidId(key);
	return getCuboidByIndex(index);
}
void AreaHasVisionCuboids::validate() const
{
	for(uint i = 0; i < m_keys.size(); ++i)
	{
		[[maybe_unused]] const VisionCuboidId& key = m_keys[i];
		[[maybe_unused]] const Cuboid& cuboid = m_cuboids[i];
		for(const auto& [adjacentKey, adjacentCuboid] : m_adjacent[i])
		{
			uint adjacentI = getIndexForVisionCuboidId(adjacentKey);
			CuboidMap<VisionCuboidId> adjacentAdjacent = m_adjacent[adjacentI];
			assert(adjacentAdjacent.contains(key));
			assert(adjacentAdjacent[key] == cuboid);
			assert(m_adjacent[i].contains(adjacentKey));
			assert(m_adjacent[i][adjacentKey] == adjacentCuboid);
		}
	}
}