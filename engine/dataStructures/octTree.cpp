#include "octTree.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../reference.h"
#include "../partitionNotify.h"
ActorOctTree::ActorOctTree(const Cuboid& cuboid)
{
	// Initalize root node;
	m_nodes.add({OctTreeIndex::null(), cuboid});
}
void ActorOctTree::record(Area& area, const ActorReference& actor)
{
	Actors& actors = area.getActors();
	const ActorIndex& index = actor.getIndex(actors.m_referenceData);
	const Point3D& location = actors.getLocation(index);
	const DistanceSquared& visionRangeSquared = actors.vision_getRangeSquared(index);
	const Facing4& facing = actors.getFacing(index);
	const VisionCuboidId& visionCuboid = area.m_visionCuboids.getVisionCuboidIndexForPoint(location);
	for(const Point3D& coordinates : Point3DSet::fromCuboidSet(actors.getOccupied(index)))
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
			int8_t octant = getOctant(node.center, coordinates);
			nodeIndex = node.children[octant];
		}
	}
}
void ActorOctTree::erase(Area& area, const ActorReference& actor)
{
	Actors& actors = area.getActors();
	const ActorIndex& index = actor.getIndex(actors.m_referenceData);
	for(const Cuboid& cuboid : actors.getOccupied(index))
		for(const Point3D& coordinates : cuboid)
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
				int8_t octant = getOctant(node.center, coordinates);
				nodeIndex = node.children[octant];
			}
		}
}
void ActorOctTree::updateVisionCuboid(const Point3D& coordinates, const VisionCuboidId& cuboid)
{
	OctTreeIndex nodeIndex = OctTreeIndex::create(0);
	while(true)
	{
		OctTreeNode& node = m_nodes[nodeIndex];
		node.contents.updateVisionCuboidIndex(coordinates, cuboid);
		if(!node.hasChildren())
			break;
		// Select new parent node at next level down.
		int8_t octant = getOctant(node.center, coordinates);
		nodeIndex = node.children[octant];
	}
}
void ActorOctTree::updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceSquared& visionRangeSquared)
{
	OctTreeIndex nodeIndex = OctTreeIndex::create(0);
	while(true)
	{
		OctTreeNode& node = m_nodes[nodeIndex];
		node.contents.updateVisionRangeSquared(actor, coordinates, visionRangeSquared);
		if(!node.hasChildren())
			break;
		// Select new parent node at next level down.
		int8_t octant = getOctant(node.center, coordinates);
		nodeIndex = node.children[octant];
	}
}
void ActorOctTree::maybeSort()
{

}
void ActorOctTree::split(const OctTreeIndex& nodeIndex)
{
	OctTreeNode& node = m_nodes[nodeIndex];
	assert(!node.hasChildren());
	node.cuboids = subdivide(node.cuboid);
	node.center = node.cuboid.getCenter();
	// Record child indices.
	OctTreeIndex childIndex = OctTreeIndex::create(m_nodes.size());
	for(int octant = 0; octant < 8; ++octant)
	{
		node.children[octant] = childIndex;
		++childIndex;
	}
	// Create children.
	for(int octant = 0; octant < 8; ++octant)
		m_nodes.emplaceBack(nodeIndex, m_nodes[nodeIndex].cuboids[octant]);
	// Copy content into newly created child nodes.
	auto locationBucketIndex = LocationBucketContentsIndex::create(0);
	// Get a fresh reference to node as the previous was invalidated by emplaceBack.
	OctTreeNode& node2 = m_nodes[nodeIndex];
	for(const Point3D& position : node2.contents.getPoints())
	{
		int8_t octant = getOctant(node2.center, position);
		// Copy whole record.
		// TODO: delay sorting till all are inserted.
		m_nodes[node2.children[octant]].contents.copyIndex(node2.contents, locationBucketIndex);
		++locationBucketIndex;
	}
}
void ActorOctTree::collapse(const OctTreeIndex& nodeIndex)
{
	OctTreeNode& node = m_nodes[nodeIndex];
	node.children.fill(OctTreeIndex::null());
	node.center.clear();
	removeNotify(m_nodes,
		[&](const OctTreeIndex& otherIndex){ return m_nodes[otherIndex].parent == nodeIndex; },
		[&](const OctTreeIndex& oldIndex, const OctTreeIndex& newIndex){
			OctTreeNode otherNode = m_nodes[oldIndex];
			// Update parent.
			OctTreeNode parent = m_nodes[otherNode.parent];
			auto found = std::ranges::find(parent.children, oldIndex);
			assert(found != parent.children.end());
			(*found) = newIndex;
			// Update children.
			if(otherNode.hasChildren())
				for(const OctTreeIndex& childIndex : otherNode.children)
					m_nodes[childIndex].parent = newIndex;
		}
	);
}
[[nodiscard]] bool ActorOctTree::contains(const ActorReference& actor, const Point3D& coordinates)
{
	return m_nodes[OctTreeIndex::create(0)].contents.contains(actor, coordinates);
}
int32_t ActorOctTree::getActorCount() const
{
	return m_nodes[OctTreeIndex::create(0)].contents.size();
}
CuboidArray<8> ActorOctTree::subdivide(const Cuboid& cuboid)
{
	// AnyFacing will do since it is a cube.
	Distance quarterWidth = cuboid.dimensionForFacing(Facing6::Above) / 4;
	Point3D center = cuboid.getCenter();
	Distance minX = center.x() - quarterWidth;
	Distance maxX = center.x() + quarterWidth;
	Distance minY = center.y() - quarterWidth;
	Distance maxY = center.y() + quarterWidth;
	Distance minZ = center.z() - quarterWidth;
	Distance maxZ = center.z() + quarterWidth;
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
int8_t ActorOctTree::getOctant(const Point3D& center, const Point3D& coordinates)
{
	return (coordinates.x() > center.x()) << 2 | (coordinates.y() > center.y()) << 1 | (coordinates.z() > center.z());
}