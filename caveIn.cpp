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
	std::list<std::unordered_set<Block*>> chunks;
	std::unordered_set<std::unordered_set<Block*>*> anchoredChunks;
	std::unordered_map<Block*, std::unordered_set<Block*>*> chunksByBlock;
	std::deque<Block*> blockQueue;

	//TODO: blockQueue.insert?
	//blockQueue.insert(blockQueue.end(), m_caveInCheck.begin(), m_caveInCheck.end());
	// For some reason the above line adds 64 elements to blockQueue rather then the expected 2.
	for(Block* block : m_caveInCheck)
		blockQueue.push_back(block);
	std::stack<Block*> toAddToBlockQueue;
	std::unordered_set<Block*> checklist(m_caveInCheck);
	m_caveInCheck.clear();
	m_caveInData.clear();
	bool chunkFound;
	bool blockIsAnchored;
	bool prioritizeAdjacent;
	while(!blockQueue.empty() && checklist.size() != 0)
	{
		Block* block = blockQueue.front();
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
		for(Block* adjacent : block->m_adjacents)
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
					std::unordered_set<Block*>* oldChunk = chunksByBlock[block];
					std::unordered_set<Block*>* newChunk = chunksByBlock[adjacent];
					for(Block* b : *oldChunk)
					{
						chunksByBlock[b] = newChunk;
						newChunk->insert(b);
					}
					// If old chunk was anchored then new chunk is as well.
					if(anchoredChunks.contains(oldChunk))
					{
						anchoredChunks.insert(newChunk);
						for(Block* b : *oldChunk)
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
			for(Block* b : *chunksByBlock[block])
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
	std::vector<std::tuple<std::unordered_set<Block*>,uint32_t>> fallingChunksWithDistance;
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
			fallingChunksWithDistance.emplace_back(chunk, smallestFallDistance);
		}
	}

	// Sort by z low to high so blocks don't overwrite eachother when moved down.
	auto compare = [](Block* a, Block* b) { return a->m_z < b->m_z; };
	for(auto& [chunk, fallDistance] : fallingChunksWithDistance)
	{
		std::vector<Block*> blocks(chunk.begin(), chunk.end());
		std::ranges::sort(blocks, compare);
		m_caveInData.emplace_back(std::move(blocks), fallDistance);
	}
}
void baseArea::stepCaveInWrite()
{
	// Make chunks fall.
	for(auto& [chunk, fallDistance] : m_caveInData)
	{
		uint32_t zDiff;
		// Move blocks down.
		Block* below;
		for(Block* block : chunk)
		{
			zDiff = fallDistance;
			below = block;
			while(zDiff)
			{
				below = below->m_adjacents[0];
				below->destroyContents();
				zDiff--;
			}
			block->moveContentsTo(below);
		}
		// We don't know if the thing we landed on was it's self anchored so add a block to caveInCheck to be checked next step.
		m_caveInCheck.insert(below);
		static_cast<Area&>(*this).afterCaveIn(chunk, fallDistance);
	}
}

void baseArea::registerPotentialCaveIn(Block& block)
{
	m_caveInCheck.insert(&block);
}
