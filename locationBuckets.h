#pragma once
class Actor;
class Block;
class Area;
class VisionRequest;
struct ActorRangeSelectors
{
	bool actor(Actor& actor);
	bool block(Block& block);
};
class LocationBuckets
{
	Area& m_area;
	std::vector<std::vector<std::vector<
		std::unordered_set<Actor*>
		>>> m_buckets;
	uint32_t m_maxX;
	uint32_t m_maxY;
	uint32_t m_maxZ;
public:
	LocationBuckets(Area& area);
	void insert(Actor* actor);
	void erase(Actor* actor);
	void update(Actor* actor, Block* oldLocation, Block* newLocation);
	std::unordered_set<Actor*> selectActorsInRange(Block& block, int32_t range, ActorRangeSelectors& selectors) const;
	void processVisionRequest(VisionRequest& visionRequest) const;
	std::unordered_set<Actor*>* getBucketFor(Block* block);
};
