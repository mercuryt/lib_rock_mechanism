#include "area.h"
#include "../blocks/blocks.h"
#include "index.h"
#include "types.h"
#include <vector>
#include <tuple>
#include <deque>
#include <stack>
#include <list>

void Area::doStepCaveIn()
{
	// Don't bother with parallizing this with anything else for now, maybe run simultaniously with something else which isn't too cache hungry at some point.
	stepCaveInRead();
	stepCaveInWrite();
}
void Area::stepCaveInRead()
{
	std::list<BlockIndices> chunks;
	SmallSet<BlockIndices*> anchoredChunks;
	BlockIndexMap<BlockIndices*> chunksByBlock;
	std::deque<BlockIndex> blockQueue;

	//TODO: blockQueue.insert?
	//blockQueue.insert(blockQueue.end(), m_caveInCheck.begin(), m_caveInCheck.end());
	// For some reason the above line adds 64 elements to blockQueue rather then the expected 2.
	for(const BlockIndex& block : m_caveInCheck)
		blockQueue.push_back(block);
	std::stack<BlockIndex> toAddToBlockQueue;
	BlockIndices checklist(m_caveInCheck);
	m_caveInCheck.clear();
	m_caveInData.clear();
	bool chunkFound;
	bool blockIsAnchored;
	bool prioritizeAdjacent;
	Blocks& blocks = getBlocks();
	while(!blockQueue.empty() && checklist.size() != 0)
	{
		BlockIndex block = blockQueue.front();
		blockQueue.pop_front();
		while(!toAddToBlockQueue.empty())
			toAddToBlockQueue.pop();
		// Already recorded.
		if(chunksByBlock.contains(block))
			continue;
		chunkFound = prioritizeAdjacent = false;
		blockIsAnchored = blocks.isEdge(block);
		// Search adjacent blocks for chunk to join.
		// We want to push_front the bottom block when no anchored chunks have been found.
		// This lets the algorithum start by trying to go straight down to establish an anchor point asap.
		// Once one point is anchored the chunks will expand in a spherical shape until they touch or anchor.
		// If this block is on the edge of the area then it is anchored.
		for(const BlockIndex& adjacent : blocks.getDirectlyAdjacent(block))
		{
			// If adjacent is not support then skip it.
			if(!blocks.isSupport(adjacent))
			{
				// Prioritize others if this is looking straight down.
				if(adjacent == blocks.getBlockBelow(block))
					prioritizeAdjacent = true;
				continue;
			}

			if(chunksByBlock.contains(adjacent))
			{
				// If adjacent to multiple different chunks merge them.
				if(chunksByBlock.contains(block))
				{
					BlockIndices* oldChunk = chunksByBlock[block];
					BlockIndices* newChunk = chunksByBlock[adjacent];
					for(const BlockIndex& b : *oldChunk)
					{
						chunksByBlock.insert(b, newChunk);
						newChunk->add(b);
					}
					// If old chunk was anchored then new chunk is as well.
					if(anchoredChunks.contains(oldChunk))
					{
						anchoredChunks.insert(newChunk);
						for(const BlockIndex& b : *oldChunk)
							checklist.remove(b);
					}
					std::erase(chunks, *oldChunk);
				}
				// Record block membership in chunk.
				chunksByBlock[adjacent]->add(block);
				chunksByBlock.insert(block, chunksByBlock[adjacent]);
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
				if(anchoredChunks.empty() && (prioritizeAdjacent || adjacent == blocks.getBlockBelow(block)))
					blockQueue.push_front(adjacent);
				else
					toAddToBlockQueue.push(adjacent);
			}
		} // End for each adjacent.

		// If no chunk found in adjacents then create a new one.
		if(!chunkFound)
		{
			chunks.push_back({block});
			chunksByBlock.insert(block, &chunks.back());
		}
		// Record anchored chunks.
		if(blockIsAnchored)
		{
			anchoredChunks.insert(chunksByBlock[block]);
			for(const BlockIndex& b : *chunksByBlock[block])
				//TODO: Why is this 'maybe'?
				checklist.maybeRemove(b);
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
	std::vector<std::tuple<BlockIndices, DistanceInBlocks, uint32_t>> fallingChunksWithDistanceAndEnergy;
	for(BlockIndices& chunk : chunks)
	{
		if(!anchoredChunks.contains(&chunk))
		{
			BlockIndices blocksAbsorbingImpact;
			DistanceInBlocks smallestFallDistance = DistanceInBlocks::create(UINT16_MAX);
			for(const BlockIndex& block : chunk)
			{
				DistanceInBlocks verticalFallDistance = DistanceInBlocks::create(0);
				BlockIndex below = blocks.getBlockBelow(block);
				while(below.exists() && !blocks.isSupport(below))
				{
					++verticalFallDistance;
					below = blocks.getBlockBelow(below);
				}
				// Ignore blocks which are not on the bottom of the shape and internal voids.
				// TODO: Allow user to do something with blocksAbsorbingImpact.
				if(verticalFallDistance > 0 && !chunk.contains(below))
				{
					if(verticalFallDistance < smallestFallDistance)
					{
						smallestFallDistance = verticalFallDistance;
						blocksAbsorbingImpact.clear();
						blocksAbsorbingImpact.add(below);
						blocksAbsorbingImpact.add(block);
					}
					else if(verticalFallDistance == smallestFallDistance)
					{
						blocksAbsorbingImpact.add(below);
						blocksAbsorbingImpact.add(block);
					}
				}
			}

			// Calculate energy of fall.
			uint32_t fallEnergy = 0;
			for(const BlockIndex& block : chunk)
				fallEnergy += blocks.solid_getMass(block).get();
			fallEnergy *= smallestFallDistance.get();

			// Store result to apply inside a write mutex after sorting.
			fallingChunksWithDistanceAndEnergy.emplace_back(chunk, smallestFallDistance, fallEnergy);
		}
	}

	// Sort by z low to high so blocks don't overwrite eachother when moved down.
	// TODO: why is it necessary to alias m_blocks like this? Clangd throws a compile error.
	auto compare = [&blocks](const BlockIndex& a, const BlockIndex& b) { return blocks.getZ(a) < blocks.getZ(b); };
	for(auto& [chunk, fallDistance, fallEnergy] : fallingChunksWithDistanceAndEnergy)
	{
		chunk.sort(compare);
		m_caveInData.emplace_back(std::move(chunk), fallDistance, fallEnergy);
	}
}
void Area::stepCaveInWrite()
{
	// Make chunks fall.
	for(auto& [chunk, fallDistance, fallEnergy] : m_caveInData)
	{
		assert(fallDistance != 0);
		assert(chunk.size());
		assert(fallEnergy);
		DistanceInBlocks zDiff;
		// Move blocks down.
		BlockIndex below;
		for(const BlockIndex& block : chunk)
		{
			zDiff = fallDistance;
			below = block;
			while(zDiff != 0)
			{
				below = getBlocks().getBlockBelow(below);
				--zDiff;
			}
			getBlocks().moveContentsTo(block, below);
		}
		// We don't know if the thing we landed on was it's self anchored so add a block to caveInCheck to be checked next step.
		m_caveInCheck.add(below);
		//TODO: disperse energy of fall by 'mining' out blocks absorbing impact
	}
}
void Area::registerPotentialCaveIn(const BlockIndex& block)
{
	m_caveInCheck.add(block);
}
