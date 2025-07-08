#include "area/area.h"
#include "../space/space.h"
#include "numericTypes/index.h"
#include "numericTypes/types.h"
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
	std::list<SmallSet<Point3D>> chunks;
	SmallSet<SmallSet<Point3D>*> anchoredChunks;
	SmallMap<Point3D, SmallSet<Point3D>*> chunksByPoint;
	std::deque<Point3D> pointQueue;

	//TODO: pointQueue.insert?
	//pointQueue.insert(pointQueue.end(), m_caveInCheck.begin(), m_caveInCheck.end());
	// For some reason the above line adds 64 elements to pointQueue rather then the expected 2.
	for(const Point3D& point : m_caveInCheck)
		pointQueue.push_back(point);
	std::stack<Point3D> toAddToPointQueue;
	SmallSet<Point3D> checklist(m_caveInCheck);
	m_caveInCheck.clear();
	m_caveInData.clear();
	bool chunkFound;
	bool pointIsAnchored;
	bool prioritizeAdjacent;
	Space& space = getSpace();
	while(!pointQueue.empty() && checklist.size() != 0)
	{
		Point3D point = pointQueue.front();
		pointQueue.pop_front();
		while(!toAddToPointQueue.empty())
			toAddToPointQueue.pop();
		// Already recorded.
		if(chunksByPoint.contains(point))
			continue;
		chunkFound = prioritizeAdjacent = false;
		pointIsAnchored = space.isEdge(point);
		// Search adjacent space for chunk to join.
		// We want to push_front the bottom point when no anchored chunks have been found.
		// This lets the algorithum start by trying to go straight down to establish an anchor point asap.
		// Once one point is anchored the chunks will expand in a spherical shape until they touch or anchor.
		// If this point is on the edge of the area then it is anchored.
		for(const Point3D& adjacent : space.getDirectlyAdjacent(point))
		{
			// If adjacent is not support then skip it.
			if(!space.isSupport(adjacent))
			{
				// Prioritize others if this is looking straight down.
				if(adjacent.below() == point)
					prioritizeAdjacent = true;
				continue;
			}

			if(chunksByPoint.contains(adjacent))
			{
				// If adjacent to multiple different chunks merge them.
				if(chunksByPoint.contains(point))
				{
					SmallSet<Point3D>* oldChunk = chunksByPoint[point];
					SmallSet<Point3D>* newChunk = chunksByPoint[adjacent];
					for(const Point3D& b : *oldChunk)
					{
						chunksByPoint.insert(b, newChunk);
						newChunk->insert(b);
					}
					// If old chunk was anchored then new chunk is as well.
					if(anchoredChunks.contains(oldChunk))
					{
						anchoredChunks.insert(newChunk);
						for(const Point3D& b : *oldChunk)
							checklist.erase(b);
					}
					chunks.remove(*oldChunk);
				}
				// Record point membership in chunk.
				chunksByPoint[adjacent]->insert(point);
				chunksByPoint.insert(point, chunksByPoint[adjacent]);
				/*
				// If the chunk is anchored then no need to do anything else.
				if(anchoredChunks.includes(chuncksByPoint[adjacent]))
					continue;
				 */
				chunkFound = true;
			}
			// If adjacent doesn't have a chunk then add it to the queue.
			else
			{
				// Put the below point (index 0) at the front of the queue until a first anchor is established.
				// If the below point is not support then prioritize all adjacent to get around the void.
				if(anchoredChunks.empty() && (prioritizeAdjacent || adjacent.below() == point))
					pointQueue.push_front(adjacent);
				else
					toAddToPointQueue.push(adjacent);
			}
		} // End for each adjacent.

		// If no chunk found in adjacents then create a new one.
		if(!chunkFound)
		{
			chunks.push_back({point});
			chunksByPoint.insert(point, &chunks.back());
		}
		// Record anchored chunks.
		if(pointIsAnchored)
		{
			anchoredChunks.insert(chunksByPoint[point]);
			for(const Point3D& b : *chunksByPoint[point])
				//TODO: Why is this 'maybe'?
				checklist.maybeErase(b);
		}
		// Append adjacent without chunks to end of pointQueue if point isn't anchored, if it is anchored then adjacent are as well.
		else if(!anchoredChunks.contains(chunksByPoint[point]))
			while(!toAddToPointQueue.empty())
			{
				pointQueue.push_back(toAddToPointQueue.top());
				toAddToPointQueue.pop();
			}

	} // End for each pointQueue.
	// Record unanchored chunks, fall distance and energy.
	std::vector<std::tuple<SmallSet<Point3D>, Distance, uint32_t>> fallingChunksWithDistanceAndEnergy;
	for(SmallSet<Point3D>& chunk : chunks)
	{
		if(!anchoredChunks.contains(&chunk))
		{
			SmallSet<Point3D> pointsAbsorbingImpact;
			Distance smallestFallDistance = Distance::create(UINT16_MAX);
			for(const Point3D& point : chunk)
			{
				Distance verticalFallDistance = Distance::create(0);
				Point3D below = point.below();
				while(below.exists() && !space.isSupport(below))
				{
					++verticalFallDistance;
					below = below.below();
				}
				// Ignore space which are not on the bottom of the shape and internal voids.
				// TODO: Allow user to do something with pointsAbsorbingImpact.
				if(verticalFallDistance > 0 && !chunk.contains(below))
				{
					if(verticalFallDistance < smallestFallDistance)
					{
						smallestFallDistance = verticalFallDistance;
						pointsAbsorbingImpact.clear();
						pointsAbsorbingImpact.insert(below);
						pointsAbsorbingImpact.insert(point);
					}
					else if(verticalFallDistance == smallestFallDistance)
					{
						pointsAbsorbingImpact.insert(below);
						pointsAbsorbingImpact.insert(point);
					}
				}
			}

			// Calculate energy of fall.
			uint32_t fallEnergy = 0;
			for(const Point3D& point : chunk)
				fallEnergy += space.solid_getMass(point).get();
			fallEnergy *= smallestFallDistance.get();

			// Store result to apply inside a write mutex after sorting.
			fallingChunksWithDistanceAndEnergy.emplace_back(chunk, smallestFallDistance, fallEnergy);
		}
	}

	// Sort by z low to high so space don't overwrite eachother when moved down.
	auto compare = [&](const Point3D& a, const Point3D& b) { return a.z() < b.z(); };
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
		Distance zDiff;
		// Move space down.
		Point3D below;
		for(const Point3D& point : chunk)
		{
			zDiff = fallDistance;
			below = point;
			while(zDiff != 0)
			{
				below = below.below();
				--zDiff;
			}
			getSpace().moveContentsTo(point, below);
		}
		// We don't know if the thing we landed on was it's self anchored so add a point to caveInCheck to be checked next step.
		m_caveInCheck.insert(below);
		//TODO: disperse energy of fall by 'mining' out space absorbing impact
	}
}
void Area::registerPotentialCaveIn(const Point3D& point)
{
	m_caveInCheck.insert(point);
}
