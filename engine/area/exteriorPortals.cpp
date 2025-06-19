
#include "exteriorPortals.h"
#include "area/area.h"

#include "../blocks/blocks.h"
#include "../definitions/moveType.h"
void AreaHasExteriorPortals::initalize(Area& area)
{
	Blocks& blocks = area.getBlocks();
	m_distances.resize(blocks.size());
	m_deltas.fill(TemperatureDelta::create(0));
	//updateAmbientSurfaceTemperature(blocks, area.m_hasTemperature.getAmbientSurfaceTemperature());
}
void AreaHasExteriorPortals::setDistance(Blocks& blocks, const BlockIndex& block, const DistanceInBlocks& distance)
{
	assert(!blocks.isExposedToSky(block));
	assert(distance.exists());
	assert(distance < m_blocks.size());
	m_blocks[distance.get()].insert(block);
	if(distance == 0)
		assert(isPortal(blocks, block));
	if(m_distances[block] == distance)
		return;
	TemperatureDelta previousDelta;
	const TemperatureDelta& newDelta = m_deltas[distance.get()];
	if(m_distances[block].exists())
		previousDelta = m_deltas[m_distances[block].get()];
	else
		previousDelta = TemperatureDelta::create(0);
	// If the newDelta is greater then previousDelta: deltaDelta will be positive.
	auto deltaDelta = newDelta - previousDelta;
	blocks.temperature_updateDelta(block, deltaDelta);
	m_distances[block] = distance;
}
void AreaHasExteriorPortals::unsetDistance(Blocks& blocks, const BlockIndex& block)
{
	const DistanceInBlocks& distance = m_distances[block];
	assert(distance.exists());
	m_blocks[distance.get()].erase(block);
	TemperatureDelta deltaDelta = m_deltas[distance.get()] * -1;
	blocks.temperature_updateDelta(block, deltaDelta);
	m_distances[block].clear();
}
void AreaHasExteriorPortals::add(Area& area, const BlockIndex& block, DistanceInBlocks distance )
{
	Blocks& blocks = area.getBlocks();
	assert(blocks.temperature_transmits(block));
	assert(!blocks.isExposedToSky(block));
	// Distance might already be set if we are regenerating after removing.
	if(m_distances[block] != distance)
		setDistance(blocks, block, distance);
	++distance;
	SmallSet<BlockIndex> currentSet;
	SmallSet<BlockIndex> nextSet;
	currentSet.insert(block);
	while(distance <= Config::maxDepthExteriorPortalPenetration && !currentSet.empty())
	{
		for(const BlockIndex& current : currentSet)
		{
			if(m_distances[current].exists() && m_distances[current] <= distance)
				continue;
			setDistance(blocks, current, distance);
			// Don't propigate through solid, but do warm / cool the exterior blocks.
			if(blocks.temperature_transmits(current))
				for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(current))
					nextSet.maybeInsert(adjacent);
		}
		currentSet.swap(nextSet);
		nextSet.clear();
		++distance;
	}
}
void AreaHasExteriorPortals::remove(Area& area, const BlockIndex& block)
{
	Blocks& blocks = area.getBlocks();
	DistanceInBlocks distance = m_distances[block];
	assert(distance.exists());
	SmallSet<BlockIndex> previousSet;
	SmallSet<BlockIndex> currentSet;
	SmallSet<BlockIndex> nextSet;
	previousSet.insert(block);
	currentSet.insert(block);
	// Record blocks created by to other portals, they will be used to regenerate some of the cleared space.
	SmallSet<BlockIndex> regenerateFrom;
	while(distance <= Config::maxDepthExteriorPortalPenetration)
	{
		for(const BlockIndex& current : currentSet)
		{
			// If a lower distance value then expected is seen it must be associated with a different source.
			// Record in regenerateFrom instead of unsetting.
			if(m_distances[current] < distance)
			{
				if(m_distances[current] != Config::maxDepthExteriorPortalPenetration && !previousSet.contains(current))
					regenerateFrom.insert(current);
				continue;
			}
			unsetDistance(blocks, current);
			// Don't propigate through solid, but do warm / cool the exterior blocks.
			if(!blocks.solid_is(current))
				for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(current))
					if(!m_distances[adjacent].empty())
						nextSet.maybeInsert(adjacent);
		}
		previousSet.swap(currentSet);
		currentSet.swap(nextSet);
		nextSet.clear();
		++distance;
	}
	for(const BlockIndex& block : regenerateFrom)
		add(area, block, m_distances[block]);
}
void AreaHasExteriorPortals::onChangeAmbiantSurfaceTemperature(Blocks& blocks, const Temperature& temperature)
{
	for(auto distance = DistanceInBlocks::create(0); distance <= Config::maxDepthExteriorPortalPenetration; ++distance)
	{
		TemperatureDelta oldDelta = m_deltas[distance.get()];
		TemperatureDelta newDelta = m_deltas[distance.get()] = getDeltaForAmbientTemperatureAndDistance(temperature, distance);
		if(newDelta == oldDelta)
			return;
		TemperatureDelta deltaDelta = newDelta - oldDelta;
		for(const BlockIndex& block : m_blocks[distance.get()])
			blocks.temperature_updateDelta(block, deltaDelta);
	}
}
void AreaHasExteriorPortals::onBlockCanTransmitTemperature(Area& area, const BlockIndex& block)
{
	DistanceInBlocks distance = m_distances[block];
	assert(distance != 0);
	Blocks& blocks = area.getBlocks();
	if(distance.empty())
	{
		if(blocks.isExposedToSky(block))
			return;
		// Check if we should create a portal here.
		bool create = false;
		DistanceInBlocks lowestAdjacentDistanceToAPortal = Config::maxDepthExteriorPortalPenetration;
		for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(block))
		{
			if(blocks.isExposedToSky(adjacent) && blocks.temperature_transmits(adjacent))
			{
				create = true;
				break;
			}
			if(m_distances[adjacent].exists() && m_distances[adjacent] < lowestAdjacentDistanceToAPortal)
				lowestAdjacentDistanceToAPortal = m_distances[adjacent];
		}
		if(!create)
		{
			// Check if we should extend a portal's infulence here.
			if(lowestAdjacentDistanceToAPortal == Config::maxDepthExteriorPortalPenetration)
				return;
			distance = lowestAdjacentDistanceToAPortal + 1;
		}
		else
			distance = DistanceInBlocks::create(0);
	}
	add(area, block, distance);
}
void AreaHasExteriorPortals::onBlockCanNotTransmitTemperature(Area& area, const BlockIndex& block)
{

	const DistanceInBlocks& distance = m_distances[block];
	if(distance.empty())
		return;
	remove(area, block);
}
TemperatureDelta AreaHasExteriorPortals::getDeltaForAmbientTemperatureAndDistance(const Temperature& ambientTemperature, const DistanceInBlocks& distance)
{
	int deltaBetweenSurfaceAndUnderground = ((int)ambientTemperature.get() - (int)Config::undergroundAmbiantTemperature.get());
	int delta = util::scaleByInverseFraction(deltaBetweenSurfaceAndUnderground, distance.get(), Config::maxDepthExteriorPortalPenetration.get() + 1);
	return TemperatureDelta::create(delta);
}
bool AreaHasExteriorPortals::isPortal(const Blocks& blocks, const BlockIndex& block)
{
	assert(blocks.temperature_transmits(block));
	if(blocks.isExposedToSky(block))
		return false;
	for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(block))
	{
		if(blocks.isExposedToSky(adjacent) && blocks.temperature_transmits(adjacent))
			return true;
	}
	return false;
}