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
	create(area, cuboid);
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
	VisionCuboid* toCombine = maybeGetTargetToCombineWith(area, cuboid);
	if(toCombine == nullptr)
		create(area, cuboid);
	else
		extend(area, *toCombine, cuboid);
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
	VisionCuboid* toCombine = maybeGetTargetToCombineWith(area, visionCuboid.m_cuboid);
	if(toCombine != nullptr)
		extend(area, *toCombine, visionCuboid.m_cuboid);
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
VisionCuboid& AreaHasVisionCuboids::create(Area& area, Cuboid& cuboid)
{
	assert(!m_blockVisionCuboidIndices.empty());
	const VisionCuboidIndex visionCuboidIndex = VisionCuboidIndex::create(m_visionCuboids.size());
	m_visionCuboids.emplaceBack(cuboid, visionCuboidIndex);
	VisionCuboid& output = m_visionCuboids.back();
	updateBlocks(area, output, visionCuboidIndex);
	return output;
}
VisionCuboid* AreaHasVisionCuboids::maybeGetTargetToCombineWith(Area& area, const Cuboid& cuboid)
{
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	auto testBlock = [&](const BlockIndex& block) -> VisionCuboid* {
		if(block.exists() && m_blockVisionCuboidIndices[block].exists())
		{
			assert(!cuboid.contains(blocks, block));
			VisionCuboid& visionCuboid = m_visionCuboids[m_blockVisionCuboidIndices[block]];
			if(!visionCuboid.m_destroy && visionCuboid.canCombineWith(area, cuboid))
				return &visionCuboid;
		}
		return nullptr;
	};
	VisionCuboid* output = nullptr;
	// Test highest against above, south, and east.
	// These are the positive directions.
	output = testBlock(blocks.getBlockAbove(cuboid.m_highest));
		if(output != nullptr)
			return output;
	output = testBlock(blocks.getBlockSouth(cuboid.m_highest));
		if(output != nullptr)
			return output;
	output = testBlock(blocks.getBlockEast(cuboid.m_highest));
		if(output != nullptr)
			return output;
	// Test lowest against below, north, and west.
	// These are the negitive directions.
	output = testBlock(blocks.getBlockBelow(cuboid.m_lowest));
		if(output != nullptr)
			return output;
	output = testBlock(blocks.getBlockNorth(cuboid.m_lowest));
		if(output != nullptr)
			return output;
	output = testBlock(blocks.getBlockWest(cuboid.m_lowest));
		if(output != nullptr)
			return output;
	return nullptr;
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
	{
		VisionCuboid* toCombine = area.m_visionCuboids.maybeGetTargetToCombineWith(area, cuboid);
		if(toCombine == nullptr)
			area.m_visionCuboids.create(area, cuboid);
		else
			extend(area, *toCombine, cuboid);
	}
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
	for(Cuboid& newCuboid : newCuboids)
	{
		assert(!newCuboid.empty(blocks));
		VisionCuboid* toCombine = area.m_visionCuboids.maybeGetTargetToCombineWith(area, newCuboid);
		if(toCombine == nullptr)
			area.m_visionCuboids.create(area, newCuboid);
		else
		{
			assert(!newCuboid.overlapsWith(blocks, toCombine->m_cuboid));
			extend(area, *toCombine, newCuboid);
		}
	}
}
// Combine and recursively search for further combinations which form cuboids.
void AreaHasVisionCuboids::extend(Area& area, VisionCuboid& visionCuboid, Cuboid& cuboid)
{
	assert(visionCuboid.m_cuboid != cuboid);
	assert(!visionCuboid.m_destroy);
	Blocks& blocks = area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	Cuboid newCuboid = visionCuboid.m_cuboid.sum(blocks, cuboid);
	VisionCuboid* toCombine = area.m_visionCuboids.maybeGetTargetToCombineWith(area, newCuboid);
	if(toCombine != nullptr)
	{
		visionCuboid.m_destroy = true;
		extend(area, *toCombine, newCuboid);
		return;
	}
	setCuboid(area, visionCuboid, newCuboid);
}
void AreaHasVisionCuboids::setCuboid(Area& area, VisionCuboid& visionCuboid, Cuboid cuboid)
{
	visionCuboid.m_cuboid = cuboid;
	area.m_visionCuboids.updateBlocks(area, visionCuboid, visionCuboid.m_index);
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
	Point3D highCoordinates = blocks.getCoordinates(m_cuboid.m_highest);
	Point3D lowCoordinates = blocks.getCoordinates(m_cuboid.m_lowest);
	Point3D otherHighCoordinates = blocks.getCoordinates(other.m_highest);
	Point3D otherLowCoordinates = blocks.getCoordinates(other.m_lowest);
	// Get a cuboid representing a face of m_cuboid.
	Facing facing = Facing::create(6);
	// Test area has x higher then this.
	if(otherLowCoordinates.x > highCoordinates.x)
		facing.set(4);
	// Test area has x lower then this.
	else if(otherHighCoordinates.x < lowCoordinates.x)
		facing.set(2);
	// Test area has y higher then this.
	else if(otherLowCoordinates.y > highCoordinates.y)
		facing.set(3);
	// Test area has y lower then this.
	else if(otherHighCoordinates.y < lowCoordinates.y)
		facing.set(1);
	// Test area has z higher then this.
	else if(otherLowCoordinates.z > highCoordinates.z)
		facing.set(5);
	// Test area has z lower then this.
	else if(otherHighCoordinates.z < lowCoordinates.z)
		facing.set(0);
	assert(facing != 6);
	const Cuboid face = m_cuboid.getFace(blocks, facing);
	std::vector<BlockIndex> faceBlocks;
	for(const BlockIndex& block : face.getView(blocks))
		faceBlocks.push_back(block);
	// Verify that the whole face can be seen through from the direction of m_cuboid.
	for(BlockIndex block : faceBlocks)
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