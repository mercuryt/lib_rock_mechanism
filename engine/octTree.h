#pragma once
#include "cuboid.h"
#include "types.h"
#include <memory>
#include <array>
template<typename T, uint splitSize>
class OctTree
{
	T m_data;
	std::unique_ptr<std::array<OctTree<T, splitSize>, 8> m_children;
	Cuboid m_cuboid;
public:
	OctTree() = default;
	OctTree(const Cuboid& cuboid) : m_cuboid(cuboid) { }
	void record(const T& value, const Point3D& coordinates)
	{
		m_data.insert(value, coordinates);
		if(m_children != nullptr)
		{
			uint8_t octant = getOctant(m_cuboid.getCenter(), coordinates);
			m_children[octant].record(value, coordinates);
		}
		else if(m_data.size() == splitSize && m_cuboid.size() >= Config::minimumSizeForOctTreeToSplit)
		{
			subdivide();
			uint8_t octant = getOctant(m_cuboid.getCenter(), coordinates);
			m_children[octant].record(value, coordinates);
		}
	}
	void subdivide()
	{
		m_children = std::make_unique<std::array<OctTree<T>, 8>();
		DistanceInBlocks halfX = m_cuboid.getSizeX() / 2;
		DistanceInBlocks halfY = m_cuboid.getSizeY() / 2;
		DistanceInBlocks halfZ = m_cuboid.getSizeZ() / 2;
		DistanceInBlocks minX = m_cuboid.getLowest().x;
		DistanceInBlocks maxX = m_cuboid.getHighest().x;
		DistanceInBlocks minY = m_cuboid.getLowest().y;
		DistanceInBlocks maxY = m_cuboid.getHighest().y;
		DistanceInBlocks minZ = m_cuboid.getLowest().z;
		DistanceInBlocks maxZ = m_cuboid.getHighest().z;
		m_children[0].setCuboid({halfX, halfY, halfZ}, {minX, minY, minZ});
		m_children[1].setCuboid({maxX, halfY, halfZ}, {halfX, minY, minZ});
		m_children[2].setCuboid({halfX, maxY, halfZ}, {minX, halfY, minZ});
		m_children[3].setCuboid({maxX, maxY, halfZ}, {halfX, halfY, minZ});
		m_children[4].setCuboid({halfX, halfY, maxZ}, {minX, minY, halfZ});
		m_children[5].setCuboid({maxX, halfY, maxZ}, {halfX, minY, halfZ});
		m_children[6].setCuboid({halfX, maxY, maxZ}, {minX, halfY, halfZ});
		m_children[7].setCuboid({maxX, maxY, maxZ}, {halfX, halfY, halfZ});
		Point3D center = m_cuboid.getCenter();
		for(const auto singleTileActorData& data : m_data.getSingleTileData)
		{
			uint8_t octant = getOctant(center, data.position);
			// Copy whole record.
			m_children[octant].insert(data);
		}
		for(const auto multiTileActorData& data : m_data.getMultiTileData)
		{
			for(const auto& [position, visionCuboid] : data.positionsAndCuboidIds)
			{
				uint8_t octant = getOctant(center, position);
				// Copy data except positions if not exist, then add position.
				m_children[octant].insert(data, position, visionCuboid);
			}
		}
	}
	void erase(const T& value, const Point3D& coordinates)
	{
		m_data.erase(value, coordinates);
		if(erased && m_children != nullptr)
		{
			if(m_data.size() < splitSize)
			{
				m_data.markLocationBucketForBlocks();
				m_children = nullptr;
			}
			else
			{
				uint8_t octant = getOctant(m_cuboid.getCenter(), position);
				m_children[octant].erase(value, coordinates);
			}
		}
	}
	template<typename Action>
	void query(const Cuboid& other, Actor&& action)
	{
		if(!other.intersects(m_cuboid))
			return;
		if(m_children == nullptr || other.contains(m_cuboid))
			action(data);
		else 
			for(const auto& child : *m_children)
				child.query(other, action);
	}
	// To be used by LocationBucket::subdivide.
	[[nodiscard]] Cuboid getCuboid() const { return m_cuboid; }
	[[nodiscard]] static uint8_t getOctant(const Point3D& center, const Point3D& other)
	{
		if(center.z >= other.z)
		{
			if(center.y >= other.y)
			{
				if(center.x >= other.x)
					return 0;
				else
					return 1;
			}
			else
			{
				if(center.x >= other.x)
					return 2;
				else
					return 3;
			}
		}
		else
		{

			if(center.y >= other.y)
			{
				if(center.x >= other.x)
					return 4;
				else
					return 5;
			}
			else
			{
				if(center.x >= other.x)
					return 6;
				else
					return 7;
			}
		}
	}
};