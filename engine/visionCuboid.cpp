#include "visionCuboid.h"
#include "area.h"
#include "types.h"
#include "blocks/blocks.h"
void AreaHasVisionCuboids::initalize(Area& area)
{
	m_area = &area;
	Blocks& blocks = area.getBlocks();
	DistanceInBlocks sizeX = blocks.m_sizeX;
	DistanceInBlocks sizeY = blocks.m_sizeY;
	DistanceInBlocks sizeZ = blocks.m_sizeZ;
	m_blockVisionCuboids.resize((sizeX * sizeY * sizeZ).get());
	m_blockVisionCuboidIds.resize((sizeX * sizeY * sizeZ).get());
	for(BlockIndex index : m_area->getBlocks().getAll())
	{
		assert(index.exists());
		if(!blocks.canSeeThrough(index))
			continue;
		Cuboid cuboid(blocks, index, index);
		VisionCuboid* toCombine = getTargetToCombineWith(cuboid);
		if(toCombine != nullptr)
			toCombine->extend(cuboid);
		else
			emplace(cuboid);
	}
	clearDestroyed();
}
void AreaHasVisionCuboids::clearDestroyed()
{
	std::erase_if(m_visionCuboids, [](VisionCuboid& visionCuboid){ return visionCuboid.m_destroy; });
}
void AreaHasVisionCuboids::blockIsNeverOpaque(const BlockIndex& block)
{
	if(m_blockVisionCuboids.empty())
		return;
	Cuboid cuboid(m_area->getBlocks(), block, block);
	VisionCuboid* toCombine = getTargetToCombineWith(cuboid);
	if(toCombine == nullptr)
		emplace(cuboid);
	else
		toCombine->extend(cuboid);
	clearDestroyed();
}
void AreaHasVisionCuboids::blockIsSometimesOpaque(const BlockIndex& block)
{
	if(m_blockVisionCuboids.empty())
		return;
	VisionCuboid* visionCuboid = m_blockVisionCuboids[block];
	assert(visionCuboid);
	visionCuboid->splitAt(block);
	clearDestroyed();
}
void AreaHasVisionCuboids::blockFloorIsNeverOpaque(const BlockIndex& block)
{
	if(m_blockVisionCuboids.empty())
		return;
	VisionCuboid* visionCuboid = m_blockVisionCuboids[block];
	assert(visionCuboid);
	VisionCuboid* toCombine = getTargetToCombineWith(visionCuboid->m_cuboid);
	if(toCombine != nullptr)
		toCombine->extend(visionCuboid->m_cuboid);
	clearDestroyed();
}
void AreaHasVisionCuboids::blockFloorIsSometimesOpaque(const BlockIndex& block)
{
	if(m_blockVisionCuboids.empty())
		return;
	VisionCuboid* visionCuboid = m_blockVisionCuboids[block];
	assert(visionCuboid);
	visionCuboid->splitBelow(block);
	clearDestroyed();
}
void AreaHasVisionCuboids::set(const BlockIndex& block, VisionCuboid& visionCuboid)
{
	if(m_blockVisionCuboids.empty())
		return;
	const VisionCuboidId oldCuboid = m_blockVisionCuboidIds[block];
	m_blockVisionCuboids[block] = &visionCuboid;
	m_blockVisionCuboidIds[block] = visionCuboid.m_id;
	// Update stored vision cuboids.
	for(const ActorIndex& actor : m_area->getBlocks().actor_getAll(block))
	{
		// Update visionCuboidId stored in VisionFacade.
		m_area->m_visionFacadeBuckets.updateCuboid(actor, visionCuboid.m_id);
		// Update visionCuboidId stored in LocationBuckets.
		m_area->m_locationBuckets.updateCuboid(*m_area, actor, block, oldCuboid, visionCuboid.m_id);
	}
	
}
void AreaHasVisionCuboids::unset(const BlockIndex& block)
{
	if(m_blockVisionCuboids.empty())
		return;
	m_blockVisionCuboids[block] = nullptr;
	m_blockVisionCuboidIds[block] = VisionCuboidId::create(0);
}
VisionCuboid& AreaHasVisionCuboids::emplace(Cuboid& cuboid)
{
	assert(!m_blockVisionCuboids.empty());
	return m_visionCuboids.emplace_back(*m_area, cuboid, m_nextId++);
}
VisionCuboid* AreaHasVisionCuboids::getTargetToCombineWith(const Cuboid& cuboid)
{
	Blocks& blocks = m_area->getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	for(BlockIndex block : blocks.getDirectlyAdjacent(cuboid.m_highest))
	{
		if(block.exists() && m_blockVisionCuboidIds[block] != 0 && !cuboid.contains(block))
		{
			VisionCuboid* visionCuboid = m_blockVisionCuboids[block];
			if(visionCuboid && !visionCuboid->m_destroy && visionCuboid->canCombineWith(cuboid))
			return visionCuboid;
		}
	}
	//if(cuboid.m_highest == cuboid.m_lowest)
	//return nullptr;
	for(BlockIndex block : blocks.getDirectlyAdjacent(cuboid.m_lowest))
		if(block.exists() && !cuboid.contains(block))
		{	
			VisionCuboid* visionCuboid = m_blockVisionCuboids[block];
			if(visionCuboid && !visionCuboid->m_destroy && visionCuboid->canCombineWith(cuboid))
			return visionCuboid;
		}
	return nullptr;
}
VisionCuboid* AreaHasVisionCuboids::getVisionCuboidFor(const BlockIndex& block)
{
	assert(!m_blockVisionCuboids.empty());
	return m_blockVisionCuboids[block];
}
VisionCuboid::VisionCuboid(Area& area, Cuboid& cuboid, VisionCuboidId id) : m_area(area), m_cuboid(cuboid), m_id(id), m_destroy(false)
{
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	for(BlockIndex block : cuboid) 
		m_area.m_visionCuboids.set(block, *this);
}
bool VisionCuboid::canCombineWith(const Cuboid& cuboid) const
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	if(!m_cuboid.canMerge(cuboid))
		return false;
	if(!canSeeInto(cuboid))
		return false;
	return true;
}
// Used when a block is no longer always transparent.
void VisionCuboid::splitAt(const BlockIndex& split)
{
	assert(!m_destroy);
	assert(m_cuboid.contains(split));
	//TODO: reuse
	m_destroy = true;
	m_area.m_visionCuboids.unset(split);
	std::vector<Cuboid> newCuboids;
	newCuboids.reserve(6);
	Blocks& blocks = m_area.getBlocks();
	Point3D splitCoordinates = blocks.getCoordinates(split);
	Point3D highCoordinates = blocks.getCoordinates(m_cuboid.m_highest);
	Point3D lowCoordinates = blocks.getCoordinates(m_cuboid.m_lowest);
	// Blocks with a lower X splt.
	if(splitCoordinates.x != lowCoordinates.x)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitCoordinates.x - 1, highCoordinates.y, highCoordinates.z}), m_cuboid.m_lowest);
	// Blocks with a higher X split.
	if(splitCoordinates.x != highCoordinates.x)
		newCuboids.emplace_back(blocks, m_cuboid.m_highest, blocks.getIndex({splitCoordinates.x + 1, lowCoordinates.y, lowCoordinates.z}));
	// Remaining blocks with a lower Y, only blocks on the same x plane as split are left avalible.
	if(splitCoordinates.y != lowCoordinates.y)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitCoordinates.x, splitCoordinates.y - 1, highCoordinates.z}), blocks.getIndex({splitCoordinates.x, lowCoordinates.y, lowCoordinates.z}));
	// Remaining blocks with a higher Y.
	if(splitCoordinates.y != highCoordinates.y)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitCoordinates.x, highCoordinates.y, highCoordinates.z}), blocks.getIndex({splitCoordinates.x, splitCoordinates.y + 1, lowCoordinates.z}));
	// Remaining blocks with a lower Z, only block on the same x and y planes as split are left avalible.
	if(splitCoordinates.z != lowCoordinates.z)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitCoordinates.x, splitCoordinates.y, splitCoordinates.z - 1}), blocks.getIndex({splitCoordinates.x, splitCoordinates.y, lowCoordinates.z}));
	// Remaining blocks with a higher Z.
	if(splitCoordinates.z != highCoordinates.z)
		newCuboids.emplace_back(blocks, blocks.getIndex({splitCoordinates.x, splitCoordinates.y, highCoordinates.z}), blocks.getIndex({splitCoordinates.x, splitCoordinates.y, splitCoordinates.z + 1}));
	for(Cuboid& cuboid : newCuboids)
	{
		VisionCuboid* toCombine = m_area.m_visionCuboids.getTargetToCombineWith(cuboid);
		if(toCombine == nullptr)
			m_area.m_visionCuboids.emplace(cuboid);
		else
			toCombine->extend(cuboid);
	}
}
// Used when a floor is no longer always transparent.
void VisionCuboid::splitBelow(const BlockIndex& split)
{
	assert(!m_destroy);
	m_destroy = true;
	//TODO: reuse
	std::vector<Cuboid> newCuboids;
	newCuboids.reserve(2);
	Blocks& blocks = m_area.getBlocks();
	Point3D highCoordinates = blocks.getCoordinates(m_cuboid.m_highest);
	Point3D lowCoordinates = blocks.getCoordinates(m_cuboid.m_lowest);
	DistanceInBlocks splitZ = blocks.getZ(split);
	// Blocks with a lower Z.
	if(splitZ != blocks.getZ(m_cuboid.m_lowest))
		newCuboids.emplace_back(blocks, blocks.getIndex({highCoordinates.x, highCoordinates.y, splitZ - 1}), m_cuboid.m_lowest);
	// Blocks with a higher Z or equal Z.
	newCuboids.emplace_back(blocks, m_cuboid.m_highest, blocks.getIndex({lowCoordinates.x, lowCoordinates.y, splitZ}));
	for(Cuboid& newCuboid : newCuboids)
	{
		assert(!newCuboid.empty());
		VisionCuboid* toCombine = m_area.m_visionCuboids.getTargetToCombineWith(newCuboid);
		if(toCombine == nullptr)
			m_area.m_visionCuboids.emplace(newCuboid);
		else
		{
			assert(!newCuboid.overlapsWith(toCombine->m_cuboid));
			toCombine->extend(newCuboid);
		}
	}
}
// Combine and recursively search for further combinations which form cuboids.
void VisionCuboid::extend(Cuboid& cuboid)
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	Blocks& blocks = m_area.getBlocks();
	assert(blocks.canSeeThrough(cuboid.m_highest));
	assert(blocks.canSeeThrough(cuboid.m_lowest));
	Cuboid newCuboid = m_cuboid.sum(cuboid);
	VisionCuboid* toCombine = m_area.m_visionCuboids.getTargetToCombineWith(newCuboid);
	if(toCombine != nullptr)
	{
		m_destroy = true;
		toCombine->extend(newCuboid);
		return;
	}
	for(BlockIndex block : cuboid) 
		m_area.m_visionCuboids.set(block, *this);
	m_cuboid = newCuboid;
}
bool VisionCuboid::canSeeInto(const Cuboid& other) const
{
	assert(m_cuboid != other);
	assert(m_cuboid.m_highest != other.m_highest);
	assert(m_cuboid.m_lowest != other.m_lowest);
	assert(!m_destroy);
	Blocks& blocks = m_area.getBlocks();
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
	const Cuboid face = m_cuboid.getFace(facing);
	std::vector<BlockIndex> faceBlocks;
	for(BlockIndex block : face)
		faceBlocks.push_back(block);
	// Verify that the whole face can be seen through from the direction of m_cuboid.
	for(BlockIndex block : face)
	{
		assert(face.contains(block));
		assert(blocks.canSeeThrough(block));
		assert(blocks.getAtFacing(block, facing).exists());
		if(!blocks.canSeeIntoFromAlways(blocks.getAtFacing(block, facing), block))
			return false;
	};
	return true;
}
