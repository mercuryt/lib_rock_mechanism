#include "area.h"
#include "../blocks/blocks.h"
#include "types.h"
#include <vector>
#include <tuple>
#include <unordered_set>
#include <deque>
#include <stack>
#include <unordered_map>
#include <list>

void Area::doStepCaveIn()
{
	// Don't bother with parallizing this with anything else for now, maybe run simultaniously with something else which isn't too cache hungry at some point.
	stepCaveInRead();
	stepCaveInWrite();
}
void Area::stepCaveInRead()
{
	std::list<std::unordered_set<BlockIndex>> chunks;
	std::unordered_set<std::unordered_set<BlockIndex>*> anchoredChunks;
	std::unordered_map<BlockIndex, std::unordered_set<BlockIndex>*> chunksByBlock;
	std::deque<BlockIndex> blockQueue;

	//TODO: blockQueue.insert?
	//blockQueue.insert(blockQueue.end(), m_caveInCheck.begin(), m_caveInCheck.end());
	// For some reason the above line adds 64 elements to blockQueue rather then the expected 2.
	for(BlockIndex block : m_caveInCheck)
		blockQueue.push_back(block);
	std::stack<BlockIndex> toAddToBlockQueue;
	std::unordered_set<BlockIndex> checklist(m_caveInCheck);
	m_caveInCheck.clear();
	m_caveInData.clear();
	bool chunkFound;
	bool blockIsAnchored;
	bool prioritizeAdjacent;
	while(!blockQueue.empty() && checklist.size() != 0)
	{
		BlockIndex block = blockQueue.front();
		blockQueue.pop_front();
		while(!toAddToBlockQueue.empty())
			toAddToBlockQueue.pop();
		// Already recorded.
		if(chunksByBlock.contains(block))
			continue;
		chunkFound = blockIsAnchored = prioritizeAdjacent = false;
		// Search adjacent blocks for chunk to join.
		// We want to push_front the bottom block when no anchored chunks have been found.
		// This lets the algorithum start by trying to go straight down to establish an anchor point asap.
		// Once one point is anchored the chunks will expand in a spherical shape until they touch or anchor.
		for(BlockIndex adjacent : m_blocks.getDirectlyAdjacent(block))
		{
			// If this block is on the edge of the area then it is anchored.
			if(adjacent == BLOCK_INDEX_MAX)
			{
				blockIsAnchored = true;
				continue;
			}

			// If adjacent is not support then skip it.
			if(!m_blocks.isSupport(adjacent))
			{
				// Prioritize others if this is looking straight down.
				if(adjacent == m_blocks.getBlockBelow(block))
					prioritizeAdjacent = true;
				continue;
			}

			if(chunksByBlock.contains(adjacent))
			{
				// If adjacent to multiple different chunks merge them.
				if(chunksByBlock.contains(block))
				{
					std::unordered_set<BlockIndex>* oldChunk = chunksByBlock[block];
					std::unordered_set<BlockIndex>* newChunk = chunksByBlock[adjacent];
					for(BlockIndex b : *oldChunk)
					{
						chunksByBlock[b] = newChunk;
						newChunk->insert(b);
					}
					// If old chunk was anchored then new chunk is as well.
					if(anchoredChunks.contains(oldChunk))
					{
						anchoredChunks.insert(newChunk);
						for(BlockIndex b : *oldChunk)
							checklist.erase(b);
					}
					std::erase(chunks, *oldChunk);
				}
				// Record block membership in chunk.
				chunksByBlock[adjacent]->insert(block);
				chunksByBlock[block] = chunksByBlock[adjacent];
				/*
				// If the chunk is anchored then no need to do anything else.
				if(anchoredChunks.includes(chuncksByBlock[adjacent]))
					continue;
				 */
				chunkFound = true;
			}
			// If adjacent doesn't have a chunk then add it to the queue.
			else 
			{
				// Put the below block (index 0) at the front of the queue until a first anchor is established.
				// If the below block is not support then prioritize all adjacent to get around the void.
				if(anchoredChunks.empty() && (prioritizeAdjacent || adjacent == m_blocks.getBlockBelow(block)))
					blockQueue.push_front(adjacent);
				else
					toAddToBlockQueue.push(adjacent);
			}
		} // End for each adjacent.

		// If no chunk found in adjacents then create a new one.
		if(!chunkFound)
		{
			chunks.push_back({block});
			chunksByBlock[block] = &chunks.back();
		}
		// Record anchored chunks.
		if(blockIsAnchored)
		{
			anchoredChunks.insert(chunksByBlock[block]);
			for(BlockIndex b : *chunksByBlock[block])
				checklist.erase(b);
		}
		// Append adjacent without chunks to end of blockQueue if block isn't anchored, if it is anchored then adjacent are as well.
		else if(!anchoredChunks.contains(chunksByBlock[block]))
			while(!toAddToBlockQueue.empty())
			{
				blockQueue.push_back(toAddToBlockQueue.top());
				toAddToBlockQueue.pop();
			}

	} // End for each blockQueue.
	// Record unanchored chunks, fall distance and energy.
	std::vector<std::tuple<std::unordered_set<BlockIndex>,uint32_t, uint32_t>> fallingChunksWithDistanceAndEnergy;
	for(std::unordered_set<BlockIndex>& chunk : chunks)
	{
		if(!anchoredChunks.contains(&chunk))
		{
			std::unordered_set<BlockIndex> blocksAbsorbingImpact;
			DistanceInBlocks smallestFallDistance = BLOCK_DISTANCE_MAX;
			for(BlockIndex block : chunk)
			{
				DistanceInBlocks verticalFallDistance = 0;
				BlockIndex below = m_blocks.getBlockBelow(block);
				while(below != BLOCK_INDEX_MAX && !m_blocks.isSupport(below))
				{
					verticalFallDistance++;
					below = m_blocks.getBlockBelow(below);
				}
				// Ignore blocks which are not on the bottom of the shape and internal voids.
				// TODO: Allow user to do something with blocksAbsorbingImpact.
				if(verticalFallDistance > 0 && !chunk.contains(below))
				{
					if(verticalFallDistance < smallestFallDistance)
					{
						smallestFallDistance = verticalFallDistance;
						blocksAbsorbingImpact.clear();
						blocksAbsorbingImpact.insert(below);
						blocksAbsorbingImpact.insert(block);
					}
					else if(verticalFallDistance == smallestFallDistance)
					{
						blocksAbsorbingImpact.insert(below);
						blocksAbsorbingImpact.insert(block);
					}
				}
			}

			// Calculate energy of fall.
			uint32_t fallEnergy = 0;
			for(BlockIndex block : chunk)
				fallEnergy += m_blocks.getMass(block);
			fallEnergy *= smallestFallDistance;
			
			// Store result to apply inside a write mutex after sorting.
			fallingChunksWithDistanceAndEnergy.emplace_back(chunk, smallestFallDistance, fallEnergy);
		}
	}

	// Sort by z low to high so blocks don't overwrite eachother when moved down.
	// TODO: why is it necessary to alias m_blocks like this? Clangd throws a compile error.
	Blocks& blocks = m_blocks;
	auto compare = [&blocks](BlockIndex a, BlockIndex b) { return blocks.getZ(a) < blocks.getZ(b); };
	for(auto& [chunk, fallDistance, fallEnergy] : fallingChunksWithDistanceAndEnergy)
	{
		std::vector<BlockIndex> blocks(chunk.begin(), chunk.end());
		std::ranges::sort(blocks, compare);
		m_caveInData.emplace_back(std::move(blocks), fallDistance, fallEnergy);
	}
}
void Area::stepCaveInWrite()
{
	// Make chunks fall.
	for(auto& [chunk, fallDistance, fallEnergy] : m_caveInData)
	{
		assert(fallDistance);
		assert(chunk.size());
		assert(fallEnergy);
		uint32_t zDiff;
		// Move blocks down.
		BlockIndex below;
		for(BlockIndex block : chunk)
		{
			zDiff = fallDistance;
			below = block;
			while(zDiff)
			{
				below = m_blocks.getBlockBelow(below);
				zDiff--;
			}
			m_blocks.moveContentsTo(block, below);
		}
		// We don't know if the thing we landed on was it's self anchored so add a block to caveInCheck to be checked next step.
		m_caveInCheck.insert(below);
		//TODO: disperse energy of fall by 'mining' out blocks absorbing impact
	}
}
void Area::registerPotentialCaveIn(BlockIndex block)
{
	m_caveInCheck.insert(block);
}
