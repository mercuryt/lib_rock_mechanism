#pragma once
// Static method.
template<class Block, class Area>
void VisionCuboid<Block, Area>::setup(Area& area)
{
	for(uint32_t x = 0; x != area.m_sizeX; ++x)
		for(uint32_t y = 0; y != area.m_sizeY; ++y)
			for(uint32_t z = 0; z != area.m_sizeZ; ++z)
			{
				Block* block = &area.m_blocks[x][y][z];
				assert(block != nullptr);
				if(!block->canSeeThrough())
					continue;
				BaseCuboid<Block> cuboid(block, block);
				VisionCuboid* toCombine = VisionCuboid::getTargetToCombineWith(cuboid);
				if(toCombine != nullptr)
					toCombine->extend(cuboid);
				else
					area.m_visionCuboids.emplace_back(cuboid);
			}
	clearDestroyed(area);
}
// Static method.

template<class Block, class Area>
void VisionCuboid<Block, Area>::clearDestroyed(Area& area){
	std::erase_if(area.m_visionCuboids, [](VisionCuboid& visionCuboid){ return visionCuboid.m_destroy; });
}
// Static method.
template<class Block, class Area>
void VisionCuboid<Block, Area>::BlockIsNeverOpaque(Block& block)
{
	BaseCuboid<Block> cuboid(block, block);
	VisionCuboid* toCombine = getTargetToCombineWith(cuboid);
	if(toCombine == nullptr)
		block.m_area->m_visionCuboids.emplace_back(cuboid);
	else
		toCombine->extend(cuboid);
}
// Static method.
template<class Block, class Area>
void VisionCuboid<Block, Area>::BlockIsSometimesOpaque(Block& block)
{
	block.m_visionCuboid->splitAt(block);
}
// Static method.
template<class Block, class Area>
void VisionCuboid<Block, Area>::BlockFloorIsNeverOpaque(Block& block)
{
	VisionCuboid* toCombine = getTargetToCombineWith(block.m_visionCuboid->m_cuboid);
	if(toCombine != nullptr)
		toCombine->extend(block.m_visionCuboid->m_cuboid);
}
// Static method.
template<class Block, class Area>
void VisionCuboid<Block, Area>::BlockFloorIsSometimesOpaque(Block& block)
{
	block.m_visionCuboid->splitBelow(block);
}
// Static method.
template<class Block, class Area>
VisionCuboid<Block, Area>* VisionCuboid<Block, Area>::getTargetToCombineWith(const BaseCuboid<Block>& cuboid)
{
	assert(cuboid.m_highest->canSeeThrough());
	assert(cuboid.m_lowest->canSeeThrough());
	for(Block* block : cuboid.m_highest->m_adjacentsVector)
		if(!cuboid.contains(*block) && block->m_visionCuboid != nullptr && !block->m_visionCuboid->m_destroy && block->m_visionCuboid->canCombineWith(cuboid))
			return block->m_visionCuboid;
	//if(cuboid.m_highest == cuboid.m_lowest)
	//return nullptr;
	for(Block* block : cuboid.m_lowest->m_adjacentsVector)
		if(!cuboid.contains(*block) && block->m_visionCuboid != nullptr && !block->m_visionCuboid->m_destroy && block->m_visionCuboid->canCombineWith(cuboid))
			return block->m_visionCuboid;
	return nullptr;
}

template<class Block, class Area>
VisionCuboid<Block, Area>::VisionCuboid(BaseCuboid<Block>& cuboid) : m_cuboid(cuboid), m_destroy(false)
{
	assert(cuboid.m_highest->canSeeThrough());
	assert(cuboid.m_lowest->canSeeThrough());
	for(Block& block : m_cuboid) { block.m_visionCuboid = this; }
}
template<class Block, class Area>
bool VisionCuboid<Block, Area>::canCombineWith(const BaseCuboid<Block>& cuboid) const
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	assert(cuboid.m_highest->canSeeThrough());
	assert(cuboid.m_lowest->canSeeThrough());
	if(!m_cuboid.canMerge(cuboid))
		return false;
	if(!canSeeInto(cuboid))
		return false;
	return true;
}
// Used when a block is no longer always transparent.
template<class Block, class Area>
void VisionCuboid<Block, Area>::splitAt(Block& split)
{
	assert(!m_destroy);
	assert(m_cuboid.contains(split));
	//TODO: reuse
	m_destroy = true;
	split.m_visionCuboid = nullptr;
	std::vector<BaseCuboid<Block>> newCuboids;
	newCuboids.reserve(6);
	// Blocks with a lower X then splt.
	if(split.m_x != m_cuboid.m_lowest->m_x)
		newCuboids.emplace_back(&split.m_area->m_blocks[split.m_x - 1][m_cuboid.m_highest->m_y][m_cuboid.m_highest->m_z], m_cuboid.m_lowest);
	// Blocks with a higher X then split.
	if(split.m_x != m_cuboid.m_highest->m_x)
		newCuboids.emplace_back(m_cuboid.m_highest, &split.m_area->m_blocks[split.m_x + 1][m_cuboid.m_lowest->m_y][m_cuboid.m_lowest->m_z]);
	// Remaining blocks with a lower Y, only blocks on the same x plane as split are left avalible.
	if(split.m_y != m_cuboid.m_lowest->m_y)
		newCuboids.emplace_back(&split.m_area->m_blocks[split.m_x][split.m_y - 1][m_cuboid.m_highest->m_z], &split.m_area->m_blocks[split.m_x][m_cuboid.m_lowest->m_y][m_cuboid.m_lowest->m_z]);
	// Remaining blocks with a higher Y.
	if(split.m_y != m_cuboid.m_highest->m_y)
		newCuboids.emplace_back(&split.m_area->m_blocks[split.m_x][m_cuboid.m_highest->m_y][m_cuboid.m_highest->m_z], &split.m_area->m_blocks[split.m_x][split.m_y + 1][m_cuboid.m_lowest->m_z]);
	// Remaining blocks with a lower Z, only block on the same x and y planes as split are left avalible.
	if(split.m_z != m_cuboid.m_lowest->m_z)
		newCuboids.emplace_back(&split.m_area->m_blocks[split.m_x][split.m_y][split.m_z - 1], &split.m_area->m_blocks[split.m_x][split.m_y][m_cuboid.m_lowest->m_z]);
	// Remaining blocks with a higher Z.
	if(split.m_z != m_cuboid.m_highest->m_z)
		newCuboids.emplace_back(&split.m_area->m_blocks[split.m_x][split.m_y][m_cuboid.m_highest->m_z], &split.m_area->m_blocks[split.m_x][split.m_y][split.m_z + 1]);
	for(BaseCuboid<Block>& cuboid : newCuboids)
	{
		VisionCuboid* toCombine = VisionCuboid::getTargetToCombineWith(cuboid);
		if(toCombine == nullptr)
			split.m_area->m_visionCuboids.emplace_back(cuboid);
		else
			toCombine->extend(cuboid);
	}
}
// Used when a floor is no longer always transparent.
template<class Block, class Area>
void VisionCuboid<Block, Area>::splitBelow(Block& split)
{
	assert(!m_destroy);
	m_destroy = true;
	//TODO: reuse
	std::vector<BaseCuboid<Block>> newCuboids;
	newCuboids.reserve(2);
	// Blocks with a lower Z.
	if(split.m_z != m_cuboid.m_lowest->m_z)
		newCuboids.emplace_back(&split.m_area->m_blocks[m_cuboid.m_highest->m_x][m_cuboid.m_highest->m_y][split.m_z - 1], m_cuboid.m_lowest);
	// Blocks with a higher Z or equal Z.
	newCuboids.emplace_back(m_cuboid.m_highest, &split.m_area->m_blocks[m_cuboid.m_lowest->m_x][m_cuboid.m_lowest->m_y][split.m_z]);
	for(BaseCuboid<Block>& newCuboid : newCuboids)
	{
		assert(!newCuboid.empty());
		VisionCuboid* toCombine = VisionCuboid::getTargetToCombineWith(newCuboid);
		if(toCombine == nullptr)
			split.m_area->m_visionCuboids.emplace_back(newCuboid);
		else
		{
			assert(!newCuboid.overlapsWith(toCombine->m_cuboid));
			toCombine->extend(newCuboid);
		}
	}
}
// Combine and recursively search for further combinations which form cuboids.
template<class Block, class Area>
void VisionCuboid<Block, Area>::extend(BaseCuboid<Block>& cuboid)
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	assert(cuboid.m_highest->canSeeThrough());
	assert(cuboid.m_lowest->canSeeThrough());
	BaseCuboid<Block> newCuboid = m_cuboid.sum(cuboid);
	VisionCuboid* toCombine = VisionCuboid::getTargetToCombineWith(newCuboid);
	if(toCombine != nullptr)
	{
		m_destroy = true;
		toCombine->extend(newCuboid);
		return;
	}
	for(Block& block : cuboid) { block.m_visionCuboid = this; }
	m_cuboid = newCuboid;
}
template<class Block, class Area>
bool VisionCuboid<Block, Area>::canSeeInto(const BaseCuboid<Block>& cuboid) const
{
	assert(m_cuboid != cuboid);
	assert(!m_destroy);
	assert(cuboid.m_highest->canSeeThrough());
	assert(cuboid.m_lowest->canSeeThrough());
	assert(m_cuboid.m_highest->canSeeThrough());
	assert(m_cuboid.m_lowest->canSeeThrough());
	// Get a cuboid representing a face of m_cuboid.
	uint32_t facing = 6;
	// Test area has x higher then this.
	if(cuboid.m_lowest->m_x > m_cuboid.m_highest->m_x)
		facing = 4;
	// Test area has x lower then this.
	else if(cuboid.m_highest->m_x < m_cuboid.m_lowest->m_x)
		facing = 2;
	// Test area has y higher then this.
	else if(cuboid.m_lowest->m_y > m_cuboid.m_highest->m_y)
		facing = 3;
	// Test area has y lower then this.
	else if(cuboid.m_highest->m_y < m_cuboid.m_lowest->m_y)
		facing = 1;
	// Test area has z higher then this.
	else if(cuboid.m_lowest->m_z > m_cuboid.m_highest->m_z)
		facing = 5;
	// Test area has z lower then this.
	else if(cuboid.m_highest->m_z < m_cuboid.m_lowest->m_z)
		facing = 0;
	assert(facing != 6);
	const BaseCuboid<Block> face = m_cuboid.getFace(facing);
	std::vector<const Block*> blocks;
	for(const Block& block : face)
		blocks.push_back(&block);
	// Verify that the whole face can be seen through from the direction of m_cuboid.
	for(const Block& block : face)
	{
		assert(face.contains(block));
		assert(block.canSeeThrough());
		assert(block.m_adjacents.at(facing) != nullptr);
		if(!block.m_adjacents[facing]->canSeeIntoFromAlways(block))
			return false;
	};
	return true;
}
