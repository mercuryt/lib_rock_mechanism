#include "octTree.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "area.h"
#include "reference.h"
#include "partitionNotify.h"
OctTree::OctTree(const Cuboid& cuboid)
{
	// Initalize root node;
	m_nodes.add({OctTreeIndex::null(), cuboid});
}
void OctTree::record(Area& area, const ActorReference& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	const ActorIndex& index = actor.getIndex(actors.m_referenceData);
	const BlockIndex& location = actors.getLocation(index);
	const DistanceInBlocks& visionRangeSquared = actors.vision_getRangeSquared(index);
	const Facing4& facing = actors.getFacing(index);
	const VisionCuboidIndex& visionCuboid = area.m_visionCuboids.getIndexForBlock(location);
	for(const Point3D& coordinates : Point3DSet::fromBlockSet(blocks, actors.getBlocks(index)))
	{
		OctTreeIndex nodeIndex = OctTreeIndex::create(0);
		while(true)
		{
			OctTreeNode& node = m_nodes[nodeIndex];
			node.contents.insert(actor, coordinates, visionCuboid, visionRangeSquared, facing);
			if(node.shouldSplit())
			{
				split(nodeIndex);
				break;
			}
			if(!node.hasChildren())
				break;
			// Select new parent node at next level down.
			uint8_t octant = getOctant(node.center, coordinates);
			nodeIndex = node.children[octant];
		}
	}
}
void OctTree::erase(Area& area, const ActorReference& actor)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	const ActorIndex& index = actor.getIndex(actors.m_referenceData);
	for(const Point3D& coordinates : Point3DSet::fromBlockSet(blocks, actors.getBlocks(index)))
	{
		OctTreeIndex nodeIndex = OctTreeIndex::create(0);
		while(true)
		{
			OctTreeNode& node = m_nodes[nodeIndex];
			node.contents.remove(actor);
			if(node.shouldMerge())
			{
				collapse(nodeIndex);
				break;
			}
			if(!node.hasChildren())
				break;
			// Select new parent node at next level down.
			uint8_t octant = getOctant(node.center, coordinates);
			nodeIndex = node.children[octant];
		}
	}
}
void OctTree::updateVisionCuboid(const Point3D& coordinates, const VisionCuboidIndex& cuboid)
{
	OctTreeIndex nodeIndex = OctTreeIndex::create(0);
	while(true)
	{
		OctTreeNode& node = m_nodes[nodeIndex];
		node.contents.updateVisionCuboidIndex(coordinates, cuboid);
		if(!node.hasChildren())
			break;
		// Select new parent node at next level down.
		uint8_t octant = getOctant(node.center, coordinates);
		nodeIndex = node.children[octant];
	}
}
void OctTree::updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared)
{
	OctTreeIndex nodeIndex = OctTreeIndex::create(0);
	while(true)
	{
		OctTreeNode& node = m_nodes[nodeIndex];
		node.contents.updateVisionRangeSquared(actor, coordinates, visionRangeSquared);
		if(!node.hasChildren())
			break;
		// Select new parent node at next level down.
		uint8_t octant = getOctant(node.center, coordinates);
		nodeIndex = node.children[octant];
	}
}
void OctTree::maybeSort()
{

}
void OctTree::split(const OctTreeIndex& nodeIndex)
{
	OctTreeNode& node = m_nodes[nodeIndex];
	assert(!node.hasChildren());
	node.cuboids = subdivide(node.cuboid);
	node.center = node.cuboid.getCenter();
	// Record child indices.
	OctTreeIndex childIndex = OctTreeIndex::create(m_nodes.size());
	for(uint8_t octant = 0; octant < 8; ++octant)
	{
		node.children[octant] = childIndex;
		++childIndex;
	}
	// Create children.
	for(uint8_t octant = 0; octant < 8; ++octant)
		m_nodes.emplaceBack(nodeIndex, m_nodes[nodeIndex].cuboids[octant]);
	// Copy content into newly created child nodes.
	auto locationBucketIndex = LocationBucketContentsIndex::create(0);
	// Get a fresh reference to node as the previous was invalidated by emplaceBack.
	OctTreeNode& node2 = m_nodes[nodeIndex];
	for(const Point3D& position : node2.contents.getPoints())
	{
		uint8_t octant = getOctant(node2.center, position);
		// Copy whole record.
		// TODO: delay sorting till all are inserted.
		m_nodes[node2.children[octant]].contents.copyIndex(node2.contents, locationBucketIndex);
		++locationBucketIndex;
	}
}
void OctTree::collapse(const OctTreeIndex& nodeIndex)
{
	OctTreeNode& node = m_nodes[nodeIndex];
	node.children.fill(OctTreeIndex::null());
	node.center.clear();
	removeNotify(m_nodes,
		[&](const OctTreeIndex& otherIndex){ return m_nodes[otherIndex].parent == nodeIndex; },
		[&](const OctTreeIndex& oldIndex, const OctTreeIndex& newIndex){
			OctTreeNode node = m_nodes[oldIndex];
			// Update parent.
			OctTreeNode parent = m_nodes[node.parent];
			auto found = std::ranges::find(parent.children, oldIndex);
			assert(found != parent.children.end());
			(*found) = newIndex;
			// Update children.
			if(node.hasChildren())
				for(const OctTreeIndex& childIndex : node.children)
					m_nodes[childIndex].parent = newIndex;
		}
	);
}
[[nodiscard]] bool OctTree::contains(const ActorReference& actor, const Point3D& coordinates)
{
	return m_nodes[OctTreeIndex::create(0)].contents.contains(actor, coordinates);
}
uint OctTree::getActorCount() const
{
	return m_nodes[OctTreeIndex::create(0)].contents.size();
}
CuboidArray<8> OctTree::subdivide(const Cuboid& cuboid)
{
	// AnyFacing will do since it is a cube.
	DistanceInBlocks quarterWidth = cuboid.dimensionForFacing(Facing6::Above) / 4;
	Point3D center = cuboid.getCenter();
	DistanceInBlocks minX = center.x() - quarterWidth;
	DistanceInBlocks maxX = center.x() + quarterWidth;
	DistanceInBlocks minY = center.y() - quarterWidth;
	DistanceInBlocks maxY = center.y() + quarterWidth;
	DistanceInBlocks minZ = center.z() - quarterWidth;
	DistanceInBlocks maxZ = center.z() + quarterWidth;
	CuboidArray<8> output;
	output.insert(0, Cuboid::createCube({minX, minY, minZ}, quarterWidth));
	output.insert(1, Cuboid::createCube({maxX, minY, minZ}, quarterWidth));
	output.insert(2, Cuboid::createCube({minX, maxY, minZ}, quarterWidth));
	output.insert(3, Cuboid::createCube({maxX, maxY, minZ}, quarterWidth));
	output.insert(4, Cuboid::createCube({minX, minY, maxZ}, quarterWidth));
	output.insert(5, Cuboid::createCube({maxX, minY, maxZ}, quarterWidth));
	output.insert(6, Cuboid::createCube({minX, maxY, maxZ}, quarterWidth));
	output.insert(7, Cuboid::createCube({maxX, maxY, maxZ}, quarterWidth));
	return output;
}
int8_t OctTree::getOctant(const Point3D& center, const Point3D& coordinates)
{
	if(center.z() >= coordinates.z())
	{
		if(center.y() >= coordinates.y())
		{
			if(center.x() >= coordinates.x())
				return 0;
			else
				return 1;
		}
		else
		{
			if(center.x() >= coordinates.x())
				return 2;
			else
				return 3;
		}
	}
	else
	{

		if(center.y() >= coordinates.y())
		{
			if(center.x() >= coordinates.x())
				return 4;
			else
				return 5;
		}
		else
		{
			if(center.x() >= coordinates.x())
				return 6;
			else
				return 7;
		}
	}
}