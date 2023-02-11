#pragma once
#include <vector>
#include <tuple>
#include <unordered_set>
#include <deque>
#include <stack>
#include <unordered_map>

class Block;

void baseArea::stepCaveInRead()
{
	std::vector<std::unordered_set<Block*>> chunks;
	std::unordered_set<std::unordered_set<Block*>*> anchoredChunks;
	std::unordered_map<Block*, std::unordered_set<Block*>*> chunksByBlock;
	//TODO: caveInCheckMutex
	std::deque<Block*> blockQueue;
	blockQueue.insert(blockQueue.begin(), m_caveInCheck.begin(), m_caveInCheck.end());
	std::stack<Block*> toAddToBlockQueue;
	std::unordered_set<Block*> checklist(m_caveInCheck);
	checklist.insert(m_caveInCheck.begin(), m_caveInCheck.end());
	bool chunkFound;
	bool blockIsAnchored;
	bool prioritizeAdjacent;
	Block* block;
	while(!blockQueue.empty() && checklist.size() != 0)
	{
		block = blockQueue.front();
		blockQueue.pop_front();
		while(!toAddToBlockQueue.empty())
			toAddToBlockQueue.pop();
		// already recorded
		if(chunksByBlock.contains(block))
			continue;
		chunkFound = blockIsAnchored = prioritizeAdjacent = false;
		// search adjacent blocks for chunk to join
		// we want to push_front the bottom block when no anchored chunks have been found
		// this lets the algorithum start by trying to go straight down to establish an anchor point asap
		// once one point is anchored the chunks will expand in a spherical shape until they touch or anchor
		for(Block* adjacent : block->m_adjacents)
		{
			// if this block is on the edge of the area then it is anchored
			if(adjacent == nullptr)
			{
				blockIsAnchored = true;
				continue;
			}

			// if adjacent is not support then skip it
			if(!adjacent->isSupport())
			{
				// prioritize others if this is looking straight down
				if(adjacent == block->m_adjacents[0])
					prioritizeAdjacent = true;
				continue;
			}

			if(chunksByBlock.contains(adjacent))
			{
				// if adjacent to multiple different chunks merge them
				if(chunksByBlock.contains(block))
				{
					std::unordered_set<Block*>* oldChunk = chunksByBlock[block];
					std::unordered_set<Block*>* newChunk = chunksByBlock[adjacent];
					for(Block* b : *oldChunk)
					{
						chunksByBlock[b] = newChunk;
						newChunk->insert(b);
						std::erase(chunks, *oldChunk);
					}
					// if old chunk was anchored then new chunk is as well
					if(anchoredChunks.contains(oldChunk))
					{
						anchoredChunks.insert(newChunk);
						//TODO: iterate checklist instead?
						for(Block* b : *oldChunk)
							checklist.erase(b);
					}
				}
				// record block membership in chunk
				chunksByBlock[adjacent]->insert(block);
				chunksByBlock[block] = chunksByBlock[adjacent];
				/*
				// if the chunk is anchored then no need to do anything else
				if(anchoredChunks.includes(chuncksByBlock[adjacent]))
					continue;
				 */
				chunkFound = true;
			}
			// if adjacent doesn't have a chunk then add it to the queue
			else 
			{
				// put the below block (index 0) at the front of the queue until a first anchor is established
				// if the below block is not support then prioritize all adjacent to get around the void
				if(anchoredChunks.empty() && (prioritizeAdjacent || adjacent == block->m_adjacents[0]))
					blockQueue.push_front(adjacent);
				else
					toAddToBlockQueue.push(adjacent);
			}
		} // end for each adjacent

		// if no chunk found in adjacents then create a new one
		if(!chunkFound)
		{
			chunks.push_back({block});
			chunksByBlock[block] = &chunks.back();
		}
		// record anchored chunks
		if(blockIsAnchored)
		{
			anchoredChunks.insert(chunksByBlock[block]);
			for(Block* b : *chunksByBlock[block])
				checklist.erase(b);
		}
		// append adjacent without chunks to end of blockQueue if block isn't anchored, if it is anchored then adjacent are as well
		else if(!anchoredChunks.contains(chunksByBlock[block]))
			while(!toAddToBlockQueue.empty())
			{
				blockQueue.push_back(toAddToBlockQueue.top());
				toAddToBlockQueue.pop();
			}

	} // end for each blockQueue
	// record unanchored chunks, fall distance and energy
	std::vector<std::tuple<std::unordered_set<Block*>,uint32_t, uint32_t>> fallingChunksWithDistanceAndEnergy;
	for(std::unordered_set<Block*>& chunk : chunks)
	{
		if(!anchoredChunks.contains(&chunk))
		{
			std::unordered_set<Block*> blocksAbsorbingImpact;
			uint32_t smallestFallDistance = UINT32_MAX;
			for(Block* block : chunk)
			{
				uint32_t verticalFallDistance = 0;
				Block* below = block->m_adjacents[0];
				while(!below->isSupport())
				{
					verticalFallDistance++;
					below = below->m_adjacents[0];
				}
				// ignore blocks which are not on the bottom of the shape and internal voids
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

			// calculate energy of fall
			uint32_t fallEnergy = 0;
			for(Block* block : chunk)
				fallEnergy += block->getMass();
			fallEnergy *= smallestFallDistance;
			
			// store result to apply inside a write mutex
			fallingChunksWithDistanceAndEnergy.emplace_back(chunk, smallestFallDistance, fallEnergy);
		}
	}

	// sort by z low to high so blocks don't overwrite eachother when moved down
	std::vector<std::tuple<std::vector<Block*>,uint32_t, uint32_t>> m_caveInData;
	auto compare = [](Block* a, Block* b) { return a->m_z < b->m_z; };
	for(auto& [chunk, fallDistance, fallEnergy] : fallingChunksWithDistanceAndEnergy)
	{
		std::vector<Block*> blocks(chunk.begin(), chunk.end());
		std::sort(blocks.begin(), blocks.end(), compare);
		m_caveInData.emplace_back(std::move(blocks), fallDistance, fallEnergy);
	}
}
void baseArea::stepCaveInWrite()
{
	// make chunks fall
	for(auto& [chunk, fallDistance, fallEnergy] : m_caveInData)
	{
		uint32_t zDiff;
		// Move blocks down.
		for(Block* block : chunk)
		{
			zDiff = fallDistance;
			Block* below = block;
			while(zDiff)
			{
				below = below->m_adjacents[0];
				zDiff--;
			}
			block->moveContentsTo(below);
		}
		//TODO: disperse energy of fall by 'mining' out blocks absorbing impact
	}
}

