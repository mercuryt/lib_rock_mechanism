#include "visionCuboid.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
void AreaHasVisionCuboids::initalize(Area& area)
{
	Blocks& blocks = area.getBlocks();
	DistanceInBlocks sizeX = blocks.m_sizeX;
	DistanceInBlocks sizeY = blocks.m_sizeY;
	DistanceInBlocks sizeZ = blocks.m_sizeZ;
	m_blockVisionCuboidIndices.resize((sizeX * sizeY * sizeZ).get());
	m_visionCuboids.reserve((float)m_blockVisionCuboidIndices.size() * Config::ratioOfVisionCuboidSlotsToReservePerBlock);
	Cuboid cuboid = blocks.getAll();
	createOrExtend(area, cuboid);
}
void AreaHasVisionCuboids::clearDestroyed(Area& area)
{
	Blocks& blocks = area.getBlocks();
	for(const VisionCuboid& visionCuboid : m_visionCuboids)
		if(visionCuboid.m_destroy)
			for(const BlockIndex& block : visionCuboid.m_cuboid.getView(blocks))
				assert(m_blockVisionCuboidIndices[block] != visionCuboid.m_index);
	m_visionCuboids.removeIf([](const VisionCuboid& visionCuboid){ return visionCuboid.m_destroy; });
	uint end = m_visionCuboids.size();
	for(auto visionCuboidIndex = VisionCuboidIndex::create(0); visionCuboidIndex < end; ++visionCuboidIndex)
	{
		VisionCuboid& visionCuboid = m_visionCuboids[visionCuboidIndex];
		if(visionCuboid.m_index != visionCuboidIndex)
		{
			updateBlocks(area, visionCuboid, visionCuboidIndex);
			visionCuboid.m_index = visionCuboidIndex;
		}
	}
}
void AreaHasVisionCuboids::blockIsNeverOpaque(Area& area, const BlockIndex& block)
{
	if(m_blockVisionCuboidIndices.empty())
		return;
	Cuboid cuboid(area.getBlocks(), block, block);
	createOrExtend(area, cuboid);
	clearDestroyed(area);
}
void AreaHasVisionCuboids::blockIsSometimesOpaque(Area& area, const BlockIndex& block)
{
	if(m_blockVisionCuboidIndices.empty())
		return;
	VisionCuboidIndex visionCuboidId = m_blockVisionCuboidIndices[block];
	assert(visionCuboidId.exists());
	VisionCuboid& visionCuboid = m_visionCuboids[visionCuboidId];
	splitAt(area, visionCuboid, block);
	clearDestroyed(area);
}
void AreaHasVisionCuboids::blockFloorIsNeverOpaque(Area& area, const BlockIndex& block)
{
	if(m_blockVisionCuboidIndices.empty())
		return;
	VisionCuboidIndex visionCuboidId = m_blockVisionCuboidIndices[block];
	assert(visionCuboidId.exists());
	VisionCuboid& visionCuboid = m_visionCuboids[visionCuboidId];
	maybeExtend(area, visionCuboid);
	clearDestroyed(area);
}
void AreaHasVisionCuboids::blockFloorIsSometimesOpaque(Area& area, const BlockIndex& block)
{
	if(m_blockVisionCuboidIndices.empty())
		return;
	VisionCuboid* visionCuboid = maybeGetForBlock(block);
	assert(visionCuboid);
	splitBelow(area, *visionCuboid, block);
	clearDestroyed(area);
}
void AreaHasVisionCuboids::cuboidIsNeverOpaque(Area& area, const Cuboid& cuboid)
{
	Blocks& blocks = area.getBlocks();
	// Some blocks may already be never opaque and thus are already part of a vision cuboid.
	// Remove those from their current cuboids so they can be incorporated into a new vision cuboid.
	SmallSet<VisionCuboidIndex> impactedVisionCuboids;
	for(const BlockIndex& block : cuboid.getView(blocks))
	{
		VisionCuboidIndex& visionIndex = m_blockVisionCuboidIndices[block];
		impactedVisionCuboids.maybeInsert(visionIndex);
		//TODO: we could be setting blockVisionCuboidIndices here, but would not be able to use createOrExtend as writen.
	}
	for(VisionCuboidIndex visionCuboidIndex : impactedVisionCuboids)
		splitAtCuboid(area, m_visionCuboids[visionCuboidIndex], cuboid);
	createOrExtend(area, cuboid);
	clearDestroyed(area);
}
void AreaHasVisionCuboids::cuboidIsSometimesOpaque(Area& area, const Cuboid& cuboid)
{
	Blocks& blocks = area.getBlocks();
	// Collect all vision cuboids which intersect with cuboid.
	SmallSet<VisionCuboidIndex> impactedVisionCuboids;
	for(const BlockIndex& block : cuboid.getView(blocks))
	{
		VisionCuboidIndex& visionIndex = m_blockVisionCuboidIndices[block];
		impactedVisionCuboids.maybeInsert(visionIndex);
		// Clear m_blockVisionCuboidIndices for all contained blocks in cuboid.
		visionIndex.clear();
	}
	// Remove all blocks contanined by cuboid from the collected cuboids.
	for(VisionCuboidIndex visionCuboidIndex : impactedVisionCuboids)
		splitAtCuboid(area, m_visionCuboids[visionCuboidIndex], cuboid);
	clearDestroyed(area);
}
void AreaHasVisionCuboids::set(Area& area, const BlockIndex& block, VisionCuboid& visionCuboid)
{
	if(m_blockVisionCuboidIndices.empty())
		return;
	assert(m_blockVisionCuboidIndices[block] != visionCuboid.m_index);
	m_blockVisionCuboidIndices[block] = visionCuboid.m_index;
	// Update stored vision cuboids.
	Actors& actors = area.getActors();
	Blocks& blocks = area.getBlocks();
	for(const ActorIndex& actor : blocks.actor_getAll(block))
	{
		// Update visionCuboidId stored in LocationBuckets.
		area.m_octTree.updateVisionCuboid(blocks.getCoordinates(block), visionCuboid.m_index);
		// Update visionCuboidId stored in VisionRequests, if any.
		actors.vision_maybeUpdateCuboid(actor, visionCuboid.m_index);
	}
}
void AreaHasVisionCuboids::unset(const BlockIndex& block)
{
	if(m_blockVisionCuboidIndices.empty())
		return;
	m_blockVisionCuboidIndices[block].clear();
}
void AreaHasVisionCuboids::createOrExtend(Area& area, const Cuboid& cuboid)
{
	Blocks& blocks = area.getBlocks();
	Cuboid currentCuboid = cuboid;
	while(true)
	{
		auto [visionCuboid, extensionCuboid] = maybeGetTargetToCombineWith(area, currentCuboid);
		if(visionCuboid == nullptr)
			break;
		if(visionCuboid->m_cuboid == extensionCuboid)
			// Whole cuboid is merged.
			visionCuboid->m_destroy = true;
		else
			// Part of cuboid is stolen.
			splitAtCuboid(area, *visionCuboid, extensionCuboid);
		currentCuboid = currentCuboid.sum(blocks, extensionCuboid);
	}
	VisionCuboidIndex index = VisionCuboidIndex::create(m_visionCuboids.size());
	m_visionCuboids.emplaceBack(currentCuboid, index);
	updateBlocks(area, m_visionCuboids.back(), index);
}
std::pair<VisionCuboid*, Cuboid> AreaHasVisionCuboids::maybeGetTargetToCombineWith(Area& area, const Cuboid& cuboid)
{
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	Cuboid cuboidOutput;
	VisionCuboid* visionCuboidOutput = nullptr;
	uint largestSize = 0;
	uint cuboidSize = cuboid.size(blocks);
	auto testBlock = [&](const BlockIndex& block) mutable {
		if(block.exists() && m_blockVisionCuboidIndices[block].exists())
		{
			assert(!cuboid.contains(blocks, block));
			VisionCuboid& otherVisionCuboid = m_visionCuboids[m_blockVisionCuboidIndices[block]];
			if(!otherVisionCuboid.m_destroy)
			{
				uint otherVisionCuboidSize = otherVisionCuboid.m_cuboid.size(blocks);
				if(otherVisionCuboidSize <= largestSize)
					return;
				if(otherVisionCuboid.canCombineWith(area, cuboid))
				{
					largestSize = otherVisionCuboidSize;
					cuboidOutput = otherVisionCuboid.m_cuboid;
					visionCuboidOutput = &otherVisionCuboid;
				}
				else if(cuboidSize >= otherVisionCuboidSize)
				{
					Cuboid potentialSteal = otherVisionCuboid.canStealFrom(area, cuboid);
					uint potentialSize = potentialSteal.size(blocks);
					if(potentialSize > largestSize)
					{
						assert(potentialSteal.canMerge(blocks, cuboid));
						largestSize = potentialSize;
						cuboidOutput = potentialSteal;
						visionCuboidOutput = &otherVisionCuboid;
					}
				}
			}
		}
	};
	// Test potential extensions in all direction, looking for the largest.
	// Test highest against above, south, and east.
	// These are the positive directions.
	testBlock(blocks.getBlockAbove(cuboid.m_highest));
	testBlock(blocks.getBlockSouth(cuboid.m_highest));
	testBlock(blocks.getBlockEast(cuboid.m_highest));
	// Test lowest against below, north, and west.
	// These are the negitive directions.
	testBlock(blocks.getBlockBelow(cuboid.m_lowest));
	testBlock(blocks.getBlockNorth(cuboid.m_lowest));
	testBlock(blocks.getBlockWest(cuboid.m_lowest));
	// If no extensions have been found these values will be returned in their null state.
	return {visionCuboidOutput, cuboidOutput};
}
VisionCuboid* AreaHasVisionCuboids::maybeGetForBlock(const BlockIndex& block)
{
	assert(block.exists());
	const VisionCuboidIndex& index = m_blockVisionCuboidIndices[block];
	if(index.empty())
		return nullptr;
	VisionCuboid* output = &m_visionCuboids[index];
	return output;
}
void AreaHasVisionCuboids::updateBlocks(Area& area, const VisionCuboid& visionCuboid, const VisionCuboidIndex& newIndex)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Cuboid cuboid = visionCuboid.m_cuboid;
	for(const BlockIndex& block : cuboid.getView(blocks))
	{
		for(const ActorIndex& actor : blocks.actor_getAll(block))
			actors.vision_maybeUpdateCuboid(actor, newIndex);
		area.m_octTree.updateVisionCuboid(blocks.getCoordinates(block), newIndex);
		m_blockVisionCuboidIndices[block] = newIndex;
	}
}
// Used when a block is no longer always transparent.
void AreaHasVisionCuboids::splitAt(Area& area, VisionCuboid& visionCuboid, const BlockIndex& split)
{
	assert(!visionCuboid.m_destroy);
	Blocks& blocks = area.getBlocks();
	assert(visionCuboid.m_cuboid.contains(blocks, split));
	//TODO: reuse
	visionCuboid.m_destroy = true;
	//TODO: what is this for?
	area.m_visionCuboids.unset(split);
	// Store for assert at end.
	Cuboid cuboid = visionCuboid.m_cuboid;
	VisionCuboidIndex index = visionCuboid.m_index;
	std::vector<Cuboid> newCuboids;
	newCuboids.reserve(6);
	Point3D splitCoordinates = blocks.getCoordinates(split);
	Point3D highCoordinates = blocks.getCoordinates(visionCuboid.m_cuboid.m_highest);
	Point3D lowCoordinates = blocks.getCoordinates(visionCuboid.m_cuboid.m_lowest);
	// Blocks with a lower Z split
	if(splitCoordinates.z != lowCoordinates.z)
		newCuboids.emplace_back(blocks, blocks.getIndex({highCoordinates.x, highCoordinates.y, splitCoordinates.z - 1}), visionCuboid.m_cuboid.m_lowest);
	// Blocks with a higher Z.
	if(splitCoordinates.z != highCoordinates.z)
		newCuboids.emplace_back(blocks, visionCuboid.m_cuboid.m_highest, blocks.getIndex({lowCoordinates.x, lowCoordinates.y, splitCoordinates.z + 1}));
	// All blocks above and below are done, work only on splitCoordinate.z level.
	// Block with a lower Y.
	if(splitCoordinates.y != lowCoordinates.y)
		newCuboids.emplace_back(blocks, blocks.getIndex({highCoordinates.x, splitCoordinates.y - 1, splitCoordinates.z}), blocks.getIndex({lowCoordinates.x, lowCoordinates.y, splitCoordinates.z}));
	// Blocks with a higher Y.
	if(splitCoordinates.y != highCoordinates.y)
		newCuboids.emplace_back(blocks, blocks.getIndex({highCoordinates.x, highCoordinates.y, splitCoordinates.z}), blocks.getIndex({lowCoordinates.x, splitCoordinates.y + 1, splitCoordinates.z}));
	// All blocks with higher or lower y are done, work only on splitCoordinate.y column.
	// Blocks with a lower X splt.
	if(splitCoordinates.x != lowCoordinates.x)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitCoordinates.x - 1, splitCoordinates.y, splitCoordinates.z}), blocks.getIndex({lowCoordinates.x, splitCoordinates.y, splitCoordinates.z}));
	// Blocks with a higher X split.
	if(splitCoordinates.x != highCoordinates.x)
		newCuboids.emplace_back(blocks, blocks.getIndex({highCoordinates.x, splitCoordinates.y, splitCoordinates.z}), blocks.getIndex({splitCoordinates.x + 1, splitCoordinates.y, splitCoordinates.z}));
	for(Cuboid& cuboid : newCuboids)
		createOrExtend(area, cuboid);
	auto cuboids = area.m_visionCuboids;
	for(const BlockIndex& block : cuboid.getView(blocks))
		assert(cuboids.getIndexForBlock(block) != index);
}
// Used when a floor is no longer always transparent.
void AreaHasVisionCuboids::splitBelow(Area& area, VisionCuboid& visionCuboid, const BlockIndex& split)
{
	assert(!visionCuboid.m_destroy);
	visionCuboid.m_destroy = true;
	//TODO: reuse
	std::vector<Cuboid> newCuboids;
	newCuboids.reserve(2);
	Blocks& blocks = area.getBlocks();
	Point3D highCoordinates = blocks.getCoordinates(visionCuboid.m_cuboid.m_highest);
	Point3D lowCoordinates = blocks.getCoordinates(visionCuboid.m_cuboid.m_lowest);
	DistanceInBlocks splitZ = blocks.getZ(split);
	// Blocks with a lower Z.
	if(splitZ != blocks.getZ(visionCuboid.m_cuboid.m_lowest))
		newCuboids.emplace_back(blocks, blocks.getIndex({highCoordinates.x, highCoordinates.y, splitZ - 1}), visionCuboid.m_cuboid.m_lowest);
	// Blocks with a higher Z or equal Z.
	newCuboids.emplace_back(blocks, visionCuboid.m_cuboid.m_highest, blocks.getIndex({lowCoordinates.x, lowCoordinates.y, splitZ}));
	for(Cuboid& cuboid : newCuboids)
		createOrExtend(area, cuboid);
}
void AreaHasVisionCuboids::splitAtCuboid(Area& area, VisionCuboid& visionCuboid, const Cuboid& splitCuboid)
{
	const Cuboid& cuboid = visionCuboid.m_cuboid;
	Blocks& blocks = area.getBlocks();
	const Point3D highest = blocks.getCoordinates(cuboid.m_highest);
	const Point3D lowest = blocks.getCoordinates(cuboid.m_lowest);
	const Point3D splitHighest = blocks.getCoordinates(splitCuboid.m_highest);
	const Point3D splitLowest = blocks.getCoordinates(splitCuboid.m_lowest);
	std::vector<Cuboid> newCuboids;
	// Split off group above.
	if(highest.z > splitHighest.z)
		newCuboids.emplace_back(blocks, cuboid.m_highest, blocks.getIndex({highest.x, highest.y, splitHighest.z + 1}));
	// Split off group below.
	if(lowest.z < splitLowest.z)
		newCuboids.emplace_back(blocks, blocks.getIndex({highest.x, highest.y, splitLowest.z - 1}), cuboid.m_lowest);
	// Split off group with higher Y
	if(highest.y > splitHighest.y)
		newCuboids.emplace_back(blocks, blocks.getIndex({highest.x, highest.y, splitHighest.z}), blocks.getIndex({lowest.x, splitHighest.y + 1, splitLowest.z}));
	// Split off group with lower Y
	if(lowest.y < splitLowest.y)
		newCuboids.emplace_back(blocks, blocks.getIndex({highest.x, splitLowest.y - 1, splitHighest.z}), blocks.getIndex({lowest.x, lowest.y, splitLowest.z}));
	// Split off group with higher X
	if(highest.x > splitHighest.x)
		newCuboids.emplace_back(blocks, blocks.getIndex({highest.x, splitHighest.y, splitHighest.z}), blocks.getIndex({splitHighest.x + 1, splitLowest.y, splitLowest.z}));
	// Split off group with lower X
	if(lowest.x < splitLowest.x)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitLowest.x - 1, splitHighest.y, splitHighest.z}), blocks.getIndex({lowest.x, splitLowest.y, splitLowest.z}));
	visionCuboid.m_destroy = true;
	for(const Cuboid& cuboid : newCuboids)
		createOrExtend(area, cuboid);
}
// Combine and recursively search for further combinations which form cuboids.
void AreaHasVisionCuboids::maybeExtend(Area& area, VisionCuboid& visionCuboid)
{
	visionCuboid.m_destroy = true;
	createOrExtend(area, visionCuboid.m_cuboid);
}
void AreaHasVisionCuboids::setCuboid(Area& area, VisionCuboid& visionCuboid, const Cuboid& cuboid)
{
	visionCuboid.m_cuboid = cuboid;
	area.m_visionCuboids.updateBlocks(area, visionCuboid, visionCuboid.m_index);
}
BlockIndex AreaHasVisionCuboids::findLowPointForCuboidStartingFromHighPoint(Area& area, const BlockIndex& highest) const
{
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(highest));
	const Point3D highestCoordinates = blocks.getCoordinates(highest);
	for(auto z = highestCoordinates.z; true; --z)
	{
		for(auto y = highestCoordinates.y; true; --y)
		{
			for(auto x = highestCoordinates.x; true; --x)
			{
				const BlockIndex& index = blocks.getIndex(x, y, z);
				if(!blocks.canSeeThrough(index))
				{
					if(z != highestCoordinates.z)
					{
						++z;
						x = blocks.m_sizeX - 1;
						y = blocks.m_sizeY - 1;
					}
					else if(y != highestCoordinates.y)
					{
						++y;
						x = blocks.m_sizeX - 1;
					}
					else
						++x;
					return blocks.getIndex({x, y, z});
				}
				if(x == 0)
					break;
			}
			if(y == 0)
				break;
		}
		if(z == 0)
			break;
	}
	return BlockIndex::null();
}
bool VisionCuboid::canSeeInto(Area& area, const Cuboid& other) const
{
	assert(m_cuboid != other);
	assert(m_cuboid.m_highest != other.m_highest);
	assert(m_cuboid.m_lowest != other.m_lowest);
	assert(!m_destroy);
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(other.m_highest));
	assert(blocks.canSeeThrough(other.m_lowest));
	// Get a cuboid representing a face of m_cuboid.
	Facing facing = m_cuboid.getFacingTwordsOtherCuboid(blocks, other);
	const Cuboid face = m_cuboid.getFace(blocks, facing);
	// Verify that the whole face can be seen through from the direction of m_cuboid.
	for(const BlockIndex& block : face.getView(blocks))
	{
		assert(face.contains(blocks, block));
		assert(blocks.canSeeThrough(block));
		assert(blocks.getAtFacing(block, facing).exists());
		if(!blocks.canSeeIntoFromAlways(blocks.getAtFacing(block, facing), block))
			return false;
	};
	return true;
}
bool VisionCuboid::canCombineWith(Area& area, const Cuboid& cuboid) const
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	if(!m_cuboid.canMerge(blocks, cuboid))
		return false;
	if(!canSeeInto(area, cuboid))
		return false;
	return true;
}
Cuboid VisionCuboid::canStealFrom(Area& area, const Cuboid& cuboid) const
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	Cuboid output = cuboid.canMergeSteal(blocks, m_cuboid);
	if(output.empty(blocks))
		return output;
	assert(output.canMerge(blocks, cuboid));
	if(!canSeeInto(area, cuboid))
		return {};
	return output;
}