#include "octTree.h"
#include "area.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
OctTree::OctTree(const DistanceInBlocks& halfWidth, Allocator& allocator) :
	m_data(allocator),
	m_cube({{halfWidth, halfWidth, halfWidth}, halfWidth}) { }
void OctTree::record(OctTreeRoot& root, const ActorReference& actor, const Point3D& coordinates, const VisionCuboidIndex& cuboid, const DistanceInBlocks& visionRangeSquared)
{
	auto& locationData = m_data.insert(actor, coordinates, cuboid, visionRangeSquared);
	afterRecord(root, locationData);
}
void OctTree::record(OctTreeRoot& root, const LocationBucketData& locationData)
{
	m_data.insert(locationData);
	afterRecord(root, locationData);
}
void OctTree::afterRecord(OctTreeRoot& root, const LocationBucketData& locationData)
{
	if(!m_children.exists() && m_data.size() == Config::minimumOccupantsForOctTreeToSplit && m_cube.size() >= Config::minimumSizeForOctTreeToSplit)
		subdivide(root);
	if(m_children.exists())
	{
		uint8_t octant = getOctant(locationData.coordinates);
		root.m_data[m_children][octant].record(root, locationData);
	}
}
void OctTree::subdivide(OctTreeRoot& root)
{
	assert(m_children.empty());
	m_children = root.allocate(*this);
	DistanceInBlocks quarterWidth = m_cube.halfWidth / 2;
	DistanceInBlocks minX = m_cube.center.x - quarterWidth;
	DistanceInBlocks maxX = m_cube.center.x + quarterWidth;
	DistanceInBlocks minY = m_cube.center.y - quarterWidth;
	DistanceInBlocks maxY = m_cube.center.y + quarterWidth;
	DistanceInBlocks minZ = m_cube.center.z - quarterWidth;
	DistanceInBlocks maxZ = m_cube.center.z + quarterWidth;
	root.m_data[m_children][0].m_cube = Cube({minX, minY, minZ}, quarterWidth);
	root.m_data[m_children][1].m_cube = Cube({maxX, minY, minZ}, quarterWidth);
	root.m_data[m_children][2].m_cube = Cube({minX, maxY, minZ}, quarterWidth);
	root.m_data[m_children][3].m_cube = Cube({maxX, maxY, minZ}, quarterWidth);
	root.m_data[m_children][4].m_cube = Cube({minX, minY, maxZ}, quarterWidth);
	root.m_data[m_children][5].m_cube = Cube({maxX, minY, maxZ}, quarterWidth);
	root.m_data[m_children][6].m_cube = Cube({minX, maxY, maxZ}, quarterWidth);
	root.m_data[m_children][7].m_cube = Cube({maxX, maxY, maxZ}, quarterWidth);
	for(const LocationBucketData& data : m_data.get())
	{
		uint8_t octant = getOctant(data.coordinates);
		// Copy whole record.
		// TODO: delay sorting till all are inserted.
		root.m_data[m_children][octant].record(root, data);
	}
}
void OctTree::erase(OctTreeRoot& root, const ActorReference& actor, const Point3D& coordinates)
{
	m_data.remove(actor);
	if(m_children.exists())
	{
		if(m_data.size() == Config::minimumOccupantsForOctTreeToUnsplit)
		{
			root.deallocate(m_children);
			m_children.clear();
		}
		else
		{
			uint8_t octant = getOctant(coordinates);
			root.m_data[m_children][octant].erase(root, actor, coordinates);
		}
	}
}
void OctTree::updateRange(OctTreeRoot& root, const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared)
{
	m_data.updateVisionRangeSquared(actor, coordinates, visionRangeSquared);
	if(m_children.exists())
	{
			uint8_t octant = getOctant(coordinates);
			root.m_data[m_children][octant].updateRange(root, actor, coordinates, visionRangeSquared);
	}
}
void OctTree::updateVisionCuboid(OctTreeRoot& root, const Point3D& coordinates, const VisionCuboidIndex& cuboid)
{
	m_data.updateVisionCuboidIndex(coordinates, cuboid);
	if(m_children.exists())
	{
			uint8_t octant = getOctant(coordinates);
			root.m_data[m_children][octant].updateVisionCuboid(root, coordinates, cuboid);
	}
}
uint8_t OctTree::getOctant(const Point3D& other)
{
	if(m_cube.center.z >= other.z)
	{
		if(m_cube.center.y >= other.y)
		{
			if(m_cube.center.x >= other.x)
				return 0;
			else
				return 1;
		}
		else
		{
			if(m_cube.center.x >= other.x)
				return 2;
			else
				return 3;
		}
	}
	else
	{

		if(m_cube.center.y >= other.y)
		{
			if(m_cube.center.x >= other.x)
				return 4;
			else
				return 5;
		}
		else
		{
			if(m_cube.center.x >= other.x)
				return 6;
			else
				return 7;
		}
	}
}
uint OctTree::getCountIncludingChildren(const OctTreeRoot& root) const
{
	uint output = 1;
	if(m_children.exists())
		for(const OctTree& child : root.m_data[m_children])
			output += child.getCountIncludingChildren(root);
	return output;
}
bool OctTree::contains(const ActorReference& actor, const Point3D& coordinates) const
{
	return m_data.contains(actor, coordinates);
}
OctTreeRoot::OctTreeRoot(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z) :
	m_tree(m_allocator)
{
	DistanceInBlocks halfWidth = std::max({x, y, z}) / 2;
	m_tree.m_cube = {{halfWidth, halfWidth, halfWidth}, halfWidth};
}
void OctTreeRoot::record(Area& area, const ActorReference& actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	DistanceInBlocks visionRangeSquared = actors.vision_getRangeSquared(index);
	for(const BlockIndex& block : actors.getBlocks(index))
		m_tree.record(*this, actor, blocks.getCoordinates(block), area.m_visionCuboids.getIndexForBlock(block), visionRangeSquared);
}
void OctTreeRoot::erase(Area& area, const ActorReference& actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	for(const BlockIndex& block : actors.getBlocks(index))
		m_tree.erase(*this, actor, blocks.getCoordinates(block));
}
OctTreeNodeIndex OctTreeRoot::allocate(OctTree& parent)
{
	OctTreeNodeIndex output = OctTreeNodeIndex::create(m_data.size());
	m_data.add({OctTree(m_allocator), OctTree(m_allocator), OctTree(m_allocator), OctTree(m_allocator), OctTree(m_allocator), OctTree(m_allocator), OctTree(m_allocator), OctTree(m_allocator)});
	m_parents.add(&parent);
	++m_entropy;
	return output;
}
void OctTreeRoot::deallocate(const OctTreeNodeIndex& index)
{
	if(index != m_data.size() - 1)
	{
		m_data[index] = m_data.back();
		m_parents[index] = m_parents.back();
		m_parents[index]->m_children = index;
	}
	m_data.popBack();
	m_parents.popBack();
	++m_entropy;
}
void OctTreeRoot::maybeSort()
{
	if(m_entropy > Config::octTreeSortEntropyThreshold)
		return;
	std::vector<std::pair<uint, OctTreeNodeIndex>> sortOrder;

	for(OctTreeNodeIndex index = OctTreeNodeIndex::create(0); index < m_data.size(); ++index)
	{
		uint order = m_parents[index]->m_cube.center.hilbertNumber();
		sortOrder.emplace_back(order, index);
		++index;
	}
	std::ranges::sort(sortOrder, {}, &std::pair<uint, OctTreeNodeIndex>::first);
	auto copyData = m_data;
	OctTreeNodeIndex index = OctTreeNodeIndex::create(0);
	for(const auto& pair : sortOrder)
	{
		m_data[index] = copyData[pair.second];
		++index;
	}
	copyData.clear();
	auto copyParents = m_parents;
	index = OctTreeNodeIndex::create(0);
	for(const auto& pair : sortOrder)
	{
		m_parents[index] = copyParents[pair.second];
		m_parents[index]->m_children = index;
		++index;
	}
	m_entropy = 0;
}