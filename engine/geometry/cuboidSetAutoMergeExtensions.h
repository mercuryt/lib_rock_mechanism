#pragma once

#include "cuboidSet.h"
#include "../dataStructures/strongVector.h"

template<typename Key>
struct CuboidSetAutoMergeMap : public CuboidSet
{
protected:
	std::vector<Key> m_keys;
	Key nextKey = Key::create(0);
public:
	virtual std::vector<std::pair<Key, Cuboid>> getAdjacentCandidates(const Cuboid&) { return std::ranges::zip(m_keys, m_cuboids); }
	virtual void onCreate(const int32_t&, const Key&, const Cuboid&) { }
	virtual void onDestroy(const int32_t&, const Key&, const Cuboid&) { }
	[[nodiscard]] virtual bool canMerge(const Cuboid&, const Cuboid&) { return true; }
	void add(const Cuboid& cuboid) override
	{
		remove(cuboid);
		for(const auto& [adjacentKey, adjacentCuboid] : getAdjacentCandidates(cuboid))
		{
			assert(!cuboid.intersects(adjacentCuboid));
			if(existing.isTouching(adjacentCuboid) && existing.canMerge(adjacentCuboid) && canMerge(adjacentCuboid, cuboid))
			{
				mergeInternal(cuboid, adjacentCuboid, adjacentKey);
				return;
			}
		}
		m_cuboids.insert(cuboid);
		m_keys.push_back(nextKey);
		++nextKey;
		onCreate(nextKey - 1, m_keys.back(), cuboid);
	}
	void remove(const Cuboid& cuboid) override
	{
		//TODO: partition instead of toSplit.
		// Record key and cuboid instead of index because index may change as keys and cuboids are destroyed.
		SmallSet<std::pair<Key, Cuboid>> toSplit;
		for(int i = 0; i < m_keys.size(); ++i)
			if(existing.intersects(m_cuboids[i]))
				toSplit.emplaceBack(m_keys[i], m_cuboids[i]);
		for(const auto& [key, cuboid] : toSplit)
		{
			int32_t index = getIndexForKey(key);
			onDestroy(index, key, cuboid);
			if(index != m_keys.size() - 1)
				m_keys[index] = m_keys.back();
			m_keys.pop_back();
			m_cuboids.erase(cuboid);
		}
		for(const auto& pair : toSplit)
			for(const Cuboid& splitResult : toSplitCuboid.getChildrenWhenSplitByCuboid(pair.second))
				create(splitResult);
	}
	virtual void sliceBelow(const Cuboid& cuboid)
	{
		assert(cuboid.dimensionForFacing(Facing6::Above) == 1);
		// Record key and cuboid instead of index because index may change as keys and cuboids are destroyed.
		SmallSet<std::pair<Key, Cuboid>> toSplit;
		for(int i = 0; i < m_keys.size(); ++i)
			if(existing.intersects(m_cuboids[i]))
				toSplit.emplaceBack(m_keys[i], m_cuboids[i]);
		for(const auto& [key, cuboid] : toSplit)
		{
			int32_t index = getIndexForKey(key);
			onDestroy(index, key, cuboid);
			if(index != m_keys.size() - 1)
				m_keys[index] = m_keys.back();
			m_keys.pop_back();
			m_cuboids.erase(cuboid);
		}
		for(const auto& pair : toSplit)
			for(const Cuboid& splitResult : toSplitCuboid.getChildrenWhenSplitBelowCuboid(pair.second))
				create(splitResult);
	}
	[[nodiscard]] Cuboid operator[](const Key& key)
	{
		auto found = std::ranges::find(m_keys, key);
		assert(found != m_keys.end());
		int32_t index = std::distance(m_keys.begin(), found);
		return m_cuboids[index];
	}
	[[nodistance]] int32_t getIndexForKey(const Key& key) { std::distance(&*m_keys.begin(), &key); }
};

#include "../numericTypes/index.h"

template<typename Key, typename Area>
struct CuboidSetAutoMergeMapWithPointLookup : public CuboidSetAutoMergeMap<Key>
{
protected:
	Area& m_area;
	RTreeData<Key> m_pointLookup;
	std::vector<std::pair<Key, Cuboid>> getAdjacentCandidates(const Cuboid& cuboid) override
	{
		Space& space = m_area.getSpace();
		std::vector<std::pair<Key,Cuboid>> output;
		std::array<Point3D, 6> candidatePoints = {
			cuboid.m_high.above(),
			cuboid.m_high.south(),
			cuboid.m_high.east(),
		};
		if(cuboid.m_low.z() != 0)
			candidatePoints[3] = cuboid.m_low.below();
		if(cuboid.m_low.z() != 0)
			candidatePoints[4] = cuboid.m_low.north();
		if(cuboid.m_low.z() != 0)
			candidatePoints[5] = cuboid.m_low.west();
		for(const Point3D& point : candidatePoints)
			if(point.exists())
			{
				const Key& key = m_pointLookup[point];
				if(key.exists())
					output.emplace_back(key, (*this)[key]);
			}
		return output;
	};
	void onCreate(const int32_t&, const Key& key, const Cuboid& cuboid) override
	{
		for(const Point3D& point : cuboid)
		{
			onKeySetForPoint(key, point);
			m_pointLookup[point] = key;
		}
	}
	void onDestroy(const int32_t&, const Key& key, const Cuboid& cuboid) override
	{
		for(const Point3D& point : cuboid)
		{
			onKeyClearForPoint(m_pointLookup[point], point);
			m_pointLookup[point].clear();
		}
	}
	virtual void onKeySetForPoint(const Key&, const Point3D&) { }
	virtual void onKeyClearForPoint(const Key&, const Point3D&) { }
public:
	CuboidSetAutoMergeMapWithPointLookup(Area& area) :
		m_area(area)
	{
		m_pointLookup.reserve( area.getSpace().size());
	}
	// To be used when setting vision cuboid in vision request and location bucket.
	[[nodiscard]] Key getKeyAt(const Point3D& point) { return m_pointLookup[point]; }
};

#include "cuboidMap.h"

template<typename Key>
struct CuboidSetAutoMergeMapWithAdjacent : public CuboidSetAutoMergeMapWithPointLookup<Key>
{
protected:
	std::vector<CuboidMap<Key>> m_adjacent;
public:
	CuboidSetAutoMergeMapWithAdjacent(Area& area) : CuboidSetAutoMergeMapWithPointLookup(area) { }
	void onCreate(const int32_t& index, const Key& key, const Cuboid& cuboid) override
	{
		CuboidSetAutoMergeMapWithPointLookup<Key>::onCreate(key, cuboid);
		const Space& space = m_area.getSpace();
		assert(index == m_adjacent.size());
		m_adjacent.emplace_back({});
		auto& adjacent = m_adjacent.back();
		for(const auto& [point, facing] : cuboid.getSurfaceView(space))
		{
			const Point3D& outside = space.getPointAtFacing(point, facing);
			Key outsideKey = m_pointLookup[outside];
			if(outsideKey.exists() && !adjacent.contains(outsideKey))
			{
				adjacent.insert(outsideKey, m_cuboids[outsideKey]);
				const int32_t outsideIndex = getIndexForKey(outsideKey);
				m_adjacent[outsideIndex].insert(key, cuboid);
			}
		}
	}
	void onDestroy(const int32_t& index, const Key& key, const Cuboid& cuboid) override
	{
		CuboidSetAutoMergeMapWithPointLookup<Key>::onDestroy(key, cuboid);
		for(const auto& [otherKey, otherCuboid] : m_adjacent[key])
		{
			int32_t otherIndex = getIndexForKey(otherKey);
			assert(m_adjacent[otherIndex][key] == cuboid);
			assert(m_adjacent[index][otherKey] == otherCuboid);
			m_adjacent[otherIndex].remove(key);
		}
		m_adjacent[index].clear();
	}
	void queryAdjacent(const auto& queryShape, const auto& action)
	{
		const Point3D center = queryShape.getCenter();
		Space& space = m_area.getSpace();
		Key key = m_pointLookup[center];
		SmallSet<Key> openList;
		SmallSet<Key> closedList;
		openList.insert(key);
		while(!openList.empty())
		{
			key = openList.back();
			openList.popBack();
			int32_t index = getIndexForKey(key);
			const Cuboid& cuboid = CuboidSet::m_cuboids[index];
			action(key, cuboid);
			const auto& adjacent = m_adjacent[index];
			for(const Key& key : adjacent.query(queryShape))
			{
				if(!closedList.contains(key))
				{
					closedList.insert(key);
					openList.insert(key);
				}
			}
		}
	}
	[[nodiscard]] Key getKeyAt(const Point3D& point) const { return CuboidSetAutoMergeMapWithPointLookup<Key>::m_pointLookup[point]; }
	[[nodiscard]] int32_t size() const { return CuboidSetAutoMergeMap<Key>::m_keys.size(); }
	[[nodiscard]] Area& getArea() { return m_area; }

	virtual void onKeySetForPoint(const Key& key, const Point3D& index) override { CuboidSetAutoMergeMapWithPointLookup<Key>::onSetKeyForPoint(key, index); }
};