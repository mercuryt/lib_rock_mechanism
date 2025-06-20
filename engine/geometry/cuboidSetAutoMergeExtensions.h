#pragma once

#include "cuboidSet.h"
#include "../dataStructures/strongVector.h"

template<typename Key>
class CuboidSetAutoMergeMap : public CuboidSet
{
protected:
	std::vector<Key> m_keys;
	Key nextKey = Key::create(0);
public:
	virtual std::vector<std::pair<Key, Cuboid>> getAdjacentCandidates(const Cuboid&) { return std::ranges::zip(m_keys, m_cuboids); }
	virtual void onCreate(const uint&, const Key&, const Cuboid&) { }
	virtual void onDestroy(const uint&, const Key&, const Cuboid&) { }
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
		for(uint i = 0; i < m_keys.size(); ++i)
			if(existing.intersects(m_cuboids[i]))
				toSplit.emplaceBack(m_keys[i], m_cuboids[i]);
		for(const auto& [key, cuboid] : toSplit)
		{
			uint index = getIndexForKey(key);
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
		for(uint i = 0; i < m_keys.size(); ++i)
			if(existing.intersects(m_cuboids[i]))
				toSplit.emplaceBack(m_keys[i], m_cuboids[i]);
		for(const auto& [key, cuboid] : toSplit)
		{
			uint index = getIndexForKey(key);
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
		uint index = std::distance(m_keys.begin(), found);
		return m_cuboids[index];
	}
	[[nodistance]] uint getIndexForKey(const Key& key) { std::distance(&*m_keys.begin(), &key); }
};

#include "../numericTypes/index.h"

template<typename Key, typename Area>
class CuboidSetAutoMergeMapWithBlockLookup : public CuboidSetAutoMergeMap<Key>
{
protected:
	Area& m_area;
	StrongVector<Key, BlockIndex> m_blockLookup;
	std::vector<std::pair<Key, Cuobid>> getAdjacentCandidates(const Cuboid& cuboid) override
	{
		Blocks& blocks = m_area.getBlocks();
		std::vector<std::pair<Key,Cuboid>> output;
		std::array<BlockIndex, 6> candidateBlocks = {
			blocks.getBlocksAbove(cuboid.m_highest),
			blocks.getBlocksSouth(cuboid.m_highest),
			blocks.getBlocksEast(cuboid.m_highest),
			blocks.getBlocksBelow(cuboid.m_lowest),
			blocks.getBlocksNorth(cuboid.m_lowest),
			blocks.getBlocksWest(cuboid.m_lowest),
		};
		for(const BlockIndex& block : candidateBlocks)
		{
			Key key = m_blockLookup[block];
			if(key.exists())
				output.emplace_back(key, (*this)[key]);
		}
		return output;
	};
	void onCreate(const uint&, const Key& key, const Cuboid& cuboid) override
	{
		for(const BlockIndex& block : cuboid.getView(m_area.getBlocks()))
		{
			onKeySetForBlock(key, block);
			m_blockLookup[block] = key;
		}
	}
	void onDestroy(const uint&, const Key& key, const Cuboid& cuboid) override
	{
		for(const BlockIndex& block : cuboid.getView(m_area.getBlocks()))
		{
			onKeyClearForBlock(m_blockLookup[block], block);
			m_blockLookup[block].clear();
		}
	}
	virtual void onKeySetForBlock(const Key&, const BlockIndex&) { }
	virtual void onKeyClearForBlock(const Key&, const BlockIndex&) { }
public:
	CuboidSetAutoMergeMapWithBlockLookup(Area& area) :
		m_area(area)
	{
		m_blockLookup.reserve(area.getBlocks().size());
	}
	// To be used when setting vision cuboid in vision request and location bucket.
	[[nodiscard]] Key getKeyAt(const BlockIndex& block) { return m_blockLookup[block]; }
};

#include "cuboidMap.h"

template<typename Key>
class CuboidSetAutoMergeMapWithAdjacent : public CuboidSetAutoMergeMapWithBlockLookup<Key>
{
protected:
	std::vector<CuboidMap<Key>> m_adjacent;
public:
	CuboidSetAutoMergeMapWithAdjacent(Area& area) : CuboidSetAutoMergeMapWithBlockLookup(area) { }
	void onCreate(const uint& index, const Key& key, const Cuboid& cuboid) override
	{
		CuboidSetAutoMergeMapWithBlockLookup<Key>::onCreate(key, cuboid);
		const Blocks& blocks = m_area.getBlocks();
		assert(index == m_adjacent.size());
		m_adjacent.emplace_back({});
		auto& adjacent = m_adjacent.back();
		for(const auto& [block, facing] : cuboid.getSurfaceView(blocks))
		{
			const BlockIndex& outside = blocks.getBlockAtFacing(block, facing);
			Key outsideKey = m_blockLookup[outside];
			if(outsideKey.exists() && !adjacent.contains(outsideKey))
			{
				adjacent.insert(outsideKey, m_cuboids[outsideKey]);
				const uint outsideIndex = getIndexForKey(outsideKey);
				m_adjacent[outsideIndex].insert(key, cuboid);
			}
		}
	}
	void onDestroy(const uint& index, const Key& key, const Cuboid& cuboid) override
	{
		CuboidSetAutoMergeMapWithBlockLookup<Key>::onDestroy(key, cuboid);
		for(const auto& [otherKey, otherCuboid] : m_adjacent[key])
		{
			uint otherIndex = getIndexForKey(otherKey);
			assert(m_adjacent[otherIndex][key] == cuboid);
			assert(m_adjacent[index][otherKey] == otherCuboid);
			m_adjacent[otherIndex].remove(key);
		}
		m_adjacent[index].clear();
	}
	void queryAdjacent(const auto& queryShape, const auto& action)
	{
		const Point3D center = queryShape.getCenter();
		Blocks& blocks = m_area.getBlocks();
		Key key = m_blockLookup[blocks.getIndex(center)];
		SmallSet<Key> openList;
		SmallSet<Key> closedList;
		openList.insert(key);
		while(!openList.empty())
		{
			key = openList.back();
			openList.popBack();
			uint index = getIndexForKey(key);
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
	[[nodiscard]] Key getKeyAt(const BlockIndex& block) const { return CuboidSetAutoMergeMapWithBlockLookup<Key>::m_blockLookup[block]; }
	[[nodiscard]] uint size() const { return CuboidSetAutoMergeMap<Key>::m_keys.size(); }
	[[nodiscard]] Area& getArea() { return m_area; }

	virtual void onKeySetForBlock(const Key& key, const BlockIndex& index) override { CuboidSetAutoMergeMapWithBlockLookup<Key>::onSetKeyForBlock(key, index); }
};