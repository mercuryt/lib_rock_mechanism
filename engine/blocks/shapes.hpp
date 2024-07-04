#include "types.h"
template<typename Key, uint8_t capacity>
class BlockHaveShapes
{
	// Store keys alone for quick access when pathing to a condition.
	std::vector<Key> keys;
	// Store pair of key / volume rather then just keys because we can usually load all of both in one cache line, for HasShape::exit().
	std::vector<std::pair<Key, CollisionVolume>> keysAndVolumes;
	BlocksHaveShapes(uint32_t size)
	{
		keys.resize(size * capacity);
		keys.fill(HAS_SHAPE_INDEX_MAX);
		keysAndVolumes.resize(size * capacity);
		keysAndVolumes.fill({HAS_SHAPE_INDEX_MAX, 0});
	}
	void insert(BlockIndex index, Key key, CollisionVolume volume)
	{
		auto begin = keys.begin() + (index * capacity);
	}
};
