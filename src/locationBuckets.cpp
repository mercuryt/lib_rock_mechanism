#include "locationBuckets.h"
#include "area.h"
#include <algorithm>
LocationBuckets::LocationBuckets(Area& area) : m_area(area)
{
	m_maxX = ((m_area.m_sizeX - 1) / Config::locationBucketSize) + 1;
	m_maxY = ((m_area.m_sizeY - 1) / Config::locationBucketSize) + 1;
	m_maxZ = ((m_area.m_sizeZ - 1) / Config::locationBucketSize) + 1;
	m_buckets.resize(m_maxX);
	for(uint32_t x = 0; x != m_maxX; ++x)
	{
		m_buckets[x].resize(m_maxY);
		for(uint32_t y = 0; y != m_maxY; ++y)
			m_buckets[x][y].resize(m_maxZ);
	}
}
std::unordered_set<Actor*>* LocationBuckets::getBucketFor(const Block& block)
{
	uint32_t bucketX = block.m_x / Config::locationBucketSize;
	uint32_t bucketY = block.m_y / Config::locationBucketSize;
	uint32_t bucketZ = block.m_z / Config::locationBucketSize;
	assert(m_buckets.size() > bucketX);
	assert(m_buckets.at(bucketX).size() > bucketY);
	assert(m_buckets.at(bucketX).at(bucketY).size() > bucketZ);
	return &m_buckets[bucketX][bucketY][bucketZ];
}
void LocationBuckets::insert(Actor& actor, Block& block)
{
	block.m_locationBucket->insert(&actor);
}
void LocationBuckets::erase(Actor& actor)
{
	assert(actor.m_location != nullptr);
	actor.m_location->m_locationBucket->erase(&actor);
}
void LocationBuckets::update(Actor& actor, const Block& oldLocation, const Block& newLocation)
{
	if(oldLocation.m_locationBucket == newLocation.m_locationBucket)
		return;
	oldLocation.m_locationBucket->erase(&actor);
	newLocation.m_locationBucket->insert(&actor);
}
void LocationBuckets::processVisionRequest(VisionRequest& visionRequest) const
{
	Block* from = visionRequest.m_actor.m_location;
	assert(from != nullptr);
	assert((float)visionRequest.m_actor.m_visionRange * Config::maxDistanceVisionModifier > 0.f);
	int32_t range = visionRequest.m_actor.m_visionRange * Config::maxDistanceVisionModifier;
	uint32_t endX = std::min(((from->m_x + range) / Config::locationBucketSize + 1), m_maxX);
	uint32_t beginX = std::max(0, (int32_t)from->m_x - range) / Config::locationBucketSize;
	uint32_t endY = std::min(((from->m_y + range) / Config::locationBucketSize + 1), m_maxY);
	uint32_t beginY = std::max(0, (int32_t)from->m_y - range) / Config::locationBucketSize;
	uint32_t endZ = std::min(((from->m_z + range) / Config::locationBucketSize + 1), m_maxZ);
	uint32_t beginZ = std::max(0, (int32_t)from->m_z - range) / Config::locationBucketSize;
	for(uint32_t x = beginX; x != endX; ++x)
		for(uint32_t y = beginY; y != endY; ++y)
			for(uint32_t z = beginZ; z != endZ; ++z)
			{
				assert(x < m_buckets.size());
				assert(y < m_buckets.at(x).size());
				assert(z < m_buckets.at(x).at(y).size());
				for(Actor* actor : m_buckets[x][y][z])
				{
					assert(!actor->m_blocks.empty());
					for(const Block* to : actor->m_blocks)
						if(to->taxiDistance(*from) <= (uint32_t)range)
						{
							if(visionRequest.hasLineOfSight(*to, *from))
								visionRequest.m_actors.insert(actor);
							continue;
						}
				}
			}
	visionRequest.m_actors.erase(&visionRequest.m_actor);
}
void LocationBuckets::getByConditionSortedByTaxiDistanceFrom(const Block& origin, std::function<bool(const Actor&)> condition, std::function<bool(const Actor&)> yield)
{
	uint32_t beginX, endX, beginY, endY, beginZ, endZ;
	beginX = endX = origin.m_x / Config::locationBucketSize;
	beginY = endY = origin.m_y / Config::locationBucketSize;
	beginZ = endZ = origin.m_z / Config::locationBucketSize;
	std::unordered_set<std::unordered_set<Actor*>*> closedList;
	while(true)
	{
		if(beginX != 0) beginX--;
		if(endX != m_maxX) endX++;
		if(beginY != 0) beginY--;
		if(endY != m_maxY) endY++;
		if(beginZ != 0) beginZ--;
		if(endZ != m_maxZ) endZ++;
		std::unordered_set<Actor*> candidates;
		for(uint32_t x = beginX; x != endX; ++x)
			for(uint32_t y = beginY; y != endY; ++y)
				for(uint32_t z = beginZ; z != endZ; ++z)
				{
					if(closedList.contains(&m_buckets[x][y][z]))
						continue;
					closedList.insert(&m_buckets[x][y][z]);
					for(Actor* actor : m_buckets[x][y][z])
						if(condition(*actor))
							candidates.insert(actor);
				}
		std::vector<Actor*> candidateVector(candidates.begin(), candidates.end());
		std::ranges::sort(candidateVector, [&](const Actor* a, const Actor* b){ return a->m_location->taxiDistance(origin) < b->m_location->taxiDistance(origin); });
		for(const Actor* const actor : candidateVector)
			// Yield actors until yield returns true.
			if(yield(*actor))
				return;
		if(beginX == 0 && beginY == 0 && beginZ == 0 && endX == m_maxX && endY == m_maxY && endZ == m_maxZ)
			return;
	}
}
LocationBuckets::InRange LocationBuckets::inRange(const Block& origin, uint32_t range) const
{
	return LocationBuckets::InRange(*this, origin, range);
}
LocationBuckets::InRange::iterator::iterator(LocationBuckets::InRange& ir) : inRange(&ir)
{
	const Block& origin = inRange->origin;
	int32_t range = inRange->range;
	maxX = (std::min(origin.m_x + range, origin.m_area->m_sizeX)) / Config::locationBucketSize;
	maxY = (std::min(origin.m_y + range, origin.m_area->m_sizeY)) / Config::locationBucketSize;
	maxZ = (std::min(origin.m_z + range, origin.m_area->m_sizeZ)) / Config::locationBucketSize;
	x = minX = (std::max((int32_t)origin.m_x - range, 0)) / Config::locationBucketSize;
	y = minY = (std::max((int32_t)origin.m_y - range, 0)) / Config::locationBucketSize;
	z = minZ = (std::max((int32_t)origin.m_z - range, 0)) / Config::locationBucketSize;
	bucket = &inRange->locationBuckets.m_buckets[minX][minY][minZ];
	bucketIterator = bucket->begin();
	findNextActor();
}
LocationBuckets::InRange::iterator::iterator() : inRange(nullptr), x(0), y(0), z(0) {}
void LocationBuckets::InRange::iterator::getNextBucket()
{
	while(bucketIterator != bucket->end())
	{
		if(++z > maxZ)
		{
			z = minZ;
			if(++y > maxY)
			{
				y = minY;
				++x;
			}
		}
		// If x > maxX then the end has been reached.
		if(x <= maxX)
		{
			bucket = &inRange->locationBuckets.m_buckets[x][y][z];
			bucketIterator = bucket->begin();
		} else
			return;
	}
}
void LocationBuckets::InRange::iterator::findNextActor()
{
	while(x <= maxX)
		if(bucketIterator == bucket->end())
			getNextBucket();
		else
			// result found, stop looping.
			break;
}
// Search through buckets for actors in range.
LocationBuckets::InRange::iterator& LocationBuckets::InRange::iterator::operator++()
{
	++bucketIterator;
	findNextActor();
	return *this;
}
LocationBuckets::InRange::iterator LocationBuckets::InRange::iterator::operator++(int) const
{
	LocationBuckets::InRange::iterator output = *this;
	++output;
	return output;
}
bool LocationBuckets::InRange::iterator::operator==(const LocationBuckets::InRange::iterator other) const { return other.x == x && other.y == y && other.z == z; }
bool LocationBuckets::InRange::iterator::operator!=(const LocationBuckets::InRange::iterator other) const { return other.x != x || other.y != y || other.z != z; }
Actor& LocationBuckets::InRange::iterator::operator*() const { return **bucketIterator; }
Actor* LocationBuckets::InRange::iterator::operator->() const { return *bucketIterator; }
LocationBuckets::InRange::iterator LocationBuckets::InRange::begin(){ return LocationBuckets::InRange::iterator(*this); }
LocationBuckets::InRange::iterator LocationBuckets::InRange::end()
{
	LocationBuckets::InRange::iterator iterator(*this);
	iterator.x = iterator.maxX + 1;
	iterator.y = iterator.maxY;
	iterator.z = iterator.maxZ;
	return iterator;
}
