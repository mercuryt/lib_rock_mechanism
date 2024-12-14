#include "octTree.h"
#include "area.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
OctTree::OctTree(const DistanceInBlocks& halfWidth) :
	m_cube({{halfWidth, halfWidth, halfWidth}, halfWidth}) { }
void OctTree::record(const ActorReference& actor, const Point3D& coordinates, const VisionCuboidId& cuboid, const DistanceInBlocks& visionRangeSquared)
{
	auto& locationData = m_data.insert(actor, coordinates, cuboid, visionRangeSquared);
	afterRecord(locationData);
}
void OctTree::record(const LocationBucketData& locationData)
{
	m_data.insert(locationData);
	afterRecord(locationData);
}
void OctTree::afterRecord(const LocationBucketData& locationData)
{
	if(m_children != nullptr)
	{
		uint8_t octant = getOctant(locationData.coordinates);
		(*m_children)[octant].record(locationData);
	}
	else if(m_data.size() == Config::minimumOccupantsForOctTreeToSplit && m_cube.size() >= Config::minimumSizeForOctTreeToSplit)
	{
		subdivide();
		uint8_t octant = getOctant(locationData.coordinates);
		(*m_children)[octant].record(locationData);
	}
}
void OctTree::subdivide()
{
	assert(m_children == nullptr);
	m_children = std::make_unique<std::array<OctTree, 8>>();
	DistanceInBlocks quarterWidth = m_cube.halfWidth / 2;
	DistanceInBlocks minX = m_cube.center.x - quarterWidth;
	DistanceInBlocks maxX = m_cube.center.x + quarterWidth;
	DistanceInBlocks minY = m_cube.center.y - quarterWidth;
	DistanceInBlocks maxY = m_cube.center.y + quarterWidth;
	DistanceInBlocks minZ = m_cube.center.z - quarterWidth;
	DistanceInBlocks maxZ = m_cube.center.z + quarterWidth;
	(*m_children)[0].m_cube = Cube({minX, minY, minZ}, quarterWidth);
	(*m_children)[1].m_cube = Cube({maxX, minY, minZ}, quarterWidth);
	(*m_children)[2].m_cube = Cube({minX, maxY, minZ}, quarterWidth);
	(*m_children)[3].m_cube = Cube({maxX, maxY, minZ}, quarterWidth);
	(*m_children)[4].m_cube = Cube({minX, minY, maxZ}, quarterWidth);
	(*m_children)[5].m_cube = Cube({maxX, minY, maxZ}, quarterWidth);
	(*m_children)[6].m_cube = Cube({minX, maxY, maxZ}, quarterWidth);
	(*m_children)[7].m_cube = Cube({maxX, maxY, maxZ}, quarterWidth);
	for(const LocationBucketData& data : m_data.get())
	{
		uint8_t octant = getOctant(data.coordinates);
		// Copy whole record.
		// TODO: delay sorting till all are inserted.
		(*m_children)[octant].record(data);
	}
}
void OctTree::erase(const ActorReference& actor, const Point3D& coordinates)
{
	m_data.remove(actor);
	if(m_children != nullptr)
	{
		if(m_data.size() == Config::minimumOccupantsForOctTreeToUnsplit)
			m_children = nullptr;
		else
		{
			uint8_t octant = getOctant(coordinates);
			(*m_children)[octant].erase(actor, coordinates);
		}
	}
}
void OctTree::updateRange(const ActorReference& actor, const Point3D& coordinates, const DistanceInBlocks& visionRangeSquared)
{
	m_data.updateVisionRangeSquared(actor, coordinates, visionRangeSquared);
	if(m_children != nullptr)
	{
			uint8_t octant = getOctant(coordinates);
			(*m_children)[octant].updateRange(actor, coordinates, visionRangeSquared);
	}
}
void OctTree::updateVisionCuboid(const Point3D& position, const VisionCuboidId& cuboid)
{
	m_data.updateVisionCuboidId(position, cuboid);
	if(m_children != nullptr)
	{
			uint8_t octant = getOctant(position);
			(*m_children)[octant].updateVisionCuboid(position, cuboid);
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
uint OctTree::getCountIncludingChildren() const
{
	uint output = 1;
	if(m_children != nullptr)
		for(const OctTree& child : *m_children)
			output += child.getCountIncludingChildren();
	return output;
}
bool OctTree::contains(const ActorReference& actor, const Point3D& coordinates) const
{
	return m_data.contains(actor, coordinates);
}
OctTreeRoot::OctTreeRoot(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z)
{
	DistanceInBlocks halfWidth = std::max({x, y, z}) / 2;
	m_data.m_cube = {{halfWidth, halfWidth, halfWidth}, halfWidth};
}
void OctTreeRoot::record(Area& area, const ActorReference& actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	DistanceInBlocks visionRangeSquared = actors.vision_getRangeSquared(index);
	for(const BlockIndex& block : actors.getBlocks(index))
		m_data.record(actor, blocks.getCoordinates(block), area.m_visionCuboids.getIdFor(block), visionRangeSquared);
}
void OctTreeRoot::erase(Area& area, const ActorReference& actor)
{
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	ActorIndex index = actor.getIndex(actors.m_referenceData);
	for(const BlockIndex& block : actors.getBlocks(index))
		m_data.erase(actor, blocks.getCoordinates(block));
}