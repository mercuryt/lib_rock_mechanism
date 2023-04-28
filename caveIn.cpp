#pragma once
#include <vector>
#include <tuple>
#include <unordered_set>
#include <deque>
#include <stack>
#include <unordered_map>

void BaseArea::stepCaveInRead()
{
	std::list<std::unordered_set<DerivedBlock*>> chunks;
	std::unordered_set<std::unordered_set<DerivedBlock*>*> anchoredChunks;
	std::unordered_map<DerivedBlock*, std::unordered_set<DerivedBlock*>*> chunksByBlock;
	std::deque<DerivedBlock*> blockQueue;

	//TODO: blockQueue.insert?
	//blockQueue.insert(blockQueue.end(), m_caveInCheck.begin(), m_caveInCheck.end());
	// For some reason the above line adds 64 elements to blockQueue rather then the expected 2.
	for(DerivedBlock* block : m_caveInCheck)
		blockQueue.push_back(block);
	std::stack<DerivedBlock*> toAddToBlockQueue;
	std::unordered_set<DerivedBlock*> checklist(m_caveInCheck);
	m_caveInCheck.clear();
	m_caveInData.clear();
	bool chunkFound;
	bool blockIsAnchored;
	bool prioritizeAdjacent;
	while(!blockQueue.empty() && checklist.size() != 0)
	{
		DerivedBlock* block = blockQueue.front();
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
		for(DerivedBlock* adjacent : block->m_adjacents)
		{
			// If this block is on the edge of the area then it is anchored.
			if(adjacent == nullptr)
			{
				blockIsAnchored = true;
				continue;
			}

			// If adjacent is not support then skip it.
			if(!adjacent->isSupport())
			{
				// Prioritize others if this is looking straight down.
				if(adjacent == block->m_adjacents[0])
					prioritizeAdjacent = true;
				continue;
			}

			if(chunksByBlock.contains(adjacent))
			{
				// If adjacent to multiple different chunks merge them.
				if(chunksByBlock.contains(block))
				{
					std::unordered_set<DerivedBlock*>* oldChunk = chunksByBlock[block];
					std::unordered_set<DerivedBlock*>* newChunk = chunksByBlock[adjacent];
					for(DerivedBlock* b : *oldChunk)
					{
						chunksByBlock[b] = newChunk;
						newChunk->insert(b);
					}
					// If old chunk was anchored then new chunk is as well.
					if(anchoredChunks.contains(oldChunk))
					{
						anchoredChunks.insert(newChunk);
						for(DerivedBlock* b : *oldChunk)
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
				if(anchoredChunks.empty() && (prioritizeAdjacent || adjacent == block->m_adjacents[0]))
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
			for(DerivedBlock* b : *chunksByBlock[block])
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
	std::vector<std::tuple<std::unordered_set<DerivedBlock*>,uint32_t, uint32_t>> fallingChunksWithDistanceAndEnergy;
	for(std::unordered_set<DerivedBlock*>& chunk : chunks)
	{
		if(!anchoredChunks.contains(&chunk))
		{
			std::unordered_set<DerivedBlock*> blocksAbsorbingImpact;
			uint32_t smallestFallDistance = UINT32_MAX;
			for(DerivedBlock* block : chunk)
			{
				uint32_t verticalFallDistance = 0;
				DerivedBlock* below = block->m_adjacents[0];
				while(below != nullptr && !below->isSupport())
				{
					verticalFallDistance++;
					below = below->m_adjacents[0];
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
			for(DerivedBlock* block : chunk)
				fallEnergy += block->getMass();
			fallEnergy *= smallestFallDistance;
			
			// Store result to apply inside a write mutex after sorting.
			fallingChunksWithDistanceAndEnergy.emplace_back(chunk, smallestFallDistance, fallEnergy);
		}
	}

	// Sort by z low to high so blocks don't overwrite eachother when moved down.
	auto compare = [](DerivedBlock* a, DerivedBlock* b) { return a->m_z < b->m_z; };
	for(auto& [chunk, fallDistance, fallEnergy] : fallingChunksWithDistanceAndEnergy)
	{
		std::vector<DerivedBlock*> blocks(chunk.begin(), chunk.end());
		std::ranges::sort(blocks, compare);
		m_caveInData.emplace_back(std::move(blocks), fallDistance, fallEnergy);
	}
}
void BaseArea::stepCaveInWrite()
{
	// Make chunks fall.
	for(auto& [chunk, fallDistance, fallEnergy] : m_caveInData)
	{
		uint32_t zDiff;
		// Move blocks down.
		DerivedBlock* below;
		for(DerivedBlock* block : chunk)
		{
			zDiff = fallDistance;
			below = block;
			while(zDiff)
			{
				below = below->m_adjacents[0];
				zDiff--;
			}
			block->moveContentsTo(below);
		}
		// We don't know if the thing we landed on was it's self anchored so add a block to caveInCheck to be checked next step.
		m_caveInCheck.insert(below);
		//TODO: disperse energy of fall by 'mining' out blocks absorbing impact
	}
}

void BaseArea::registerPotentialCaveIn(DerivedBlock& block)
{
	m_caveInCheck.insert(&block);
}
