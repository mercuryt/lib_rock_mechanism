#include "items.h"
#include "../definitions/moveType.h"
#include "../definitions/shape.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../fluidType.h"
void Items::pilot_set(const ItemIndex& item, const ActorIndex& pilot)
{
	assert(m_pilot[item].empty());
	assert(m_onDeck[item].contains(ActorOrItemIndex::createForActor(pilot)));
	m_pilot[item] = pilot;
	unsetStatic(item);
}
void Items::pilot_clear(const ItemIndex& item)
{
	assert(m_pilot[item].exists());
	m_pilot[item].clear();
	setStatic(item);
}
ActorIndex Items::pilot_get(const ItemIndex& item) const
{
	return m_pilot[item];
}
Force Items::vehicle_getMotiveForce(const ItemIndex& item) const
{
	// Force from engine / sails / etc.
	Force builtInMotiveForce = ItemType::getMotiveForce(m_itemType[item]);
	if(builtInMotiveForce != 0)
		return builtInMotiveForce;
	// No engines or sails, check rowing.
	AttributeLevel sumOfStrength = AttributeLevel::create(0);
	const Actors& actors = m_area.getActors();
	if(isFloating(item))
	{
		for(const ActorOrItemIndex& onDeck : onDeck_get(item))
			if(onDeck.isActor())
				//TODO: check for rowing objective.
				sumOfStrength += actors.getStrength(onDeck.getActor());
		if(sumOfStrength != 0)
			return Force::create(sumOfStrength.get() * Config::rowForcePerUnitStrength);
	}
	return Force::create(0);
}
Speed Items::vehicle_getSpeed(const ItemIndex& item) const
{
	Force motiveForce = vehicle_getMotiveForce(item);
	Speed baseSpeed = motiveForce / getMass(item);
	const FluidTypeId& floatingInFluidType = m_floating[item];
	if(floatingInFluidType.exists())
	{
		const ShapeId& shape = getShape(item);
		const auto& level = floatsInAtDepth(item, floatingInFluidType);
		Quantity numberOfBlocksOnLeadingFaceAtOrBelowFloatLevel = Shape::getNumberOfBlocksOnLeadingFaceAtOrBelowLevel(shape, level);
		Speed drag = Speed::create(
			numberOfBlocksOnLeadingFaceAtOrBelowFloatLevel.get() *
			FluidType::getDensity(floatingInFluidType).get() *
			baseSpeed.get() *
			Config::fluidDragModifier
		);
		return baseSpeed - drag;
	}
	return baseSpeed;
}
const OffsetCuboidSet& Items::getDeckOffsets(const ItemIndex& index) const { return ItemType::getDecks(m_itemType[index]); }