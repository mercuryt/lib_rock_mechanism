#include "portables.hpp"


// Class method.
Speed PortablesHelpers::getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems)
{
	return getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), Mass::create(0), Mass::create(0));
}
Speed PortablesHelpers::getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, const Mass& addedRollingMass, const Mass& addedFloatingMass, const Mass& addedDeadMass)
{
	Mass rollingMass = addedRollingMass;
	Mass floatingMass = addedFloatingMass;
	Mass deadMass = addedDeadMass;
	Mass carryMass = Mass::create(0);
	Speed lowestMoveSpeed = Speed::create(0);
	const Items& items = area.getItems();
	const Actors& actors = area.getActors();
	for(ActorOrItemIndex index : actorsAndItems)
	{
		if(index.isItem())
		{
			const ItemIndex& itemIndex = ItemIndex::cast(index.get());
			const ActorIndex& pilot = items.pilot_get(itemIndex);
			Mass mass = items.getMass(itemIndex);
			if(pilot.exists())
			{
				Speed moveSpeed = actors.move_getSpeed(pilot);
				lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
				carryMass += mass * Config::vehicleMassToCarryMassModifier;
			}
			else
			{
				static MoveTypeId roll = MoveType::byName("roll");
				static MoveTypeId floating = MoveType::byName("floating");
				const MoveTypeId& moveType = items.getMoveType(itemIndex);
				if(moveType == floating)
					floatingMass += mass;
				else if(items.getMoveType(itemIndex) == roll)
					rollingMass += mass;
				else
					deadMass += mass;
			}
		}
		else
		{
			assert(index.isActor());
			const ActorIndex& actorIndex = ActorIndex::cast(index.get());
			if(actors.move_canMove(actorIndex))
			{
				carryMass += actors.getUnencomberedCarryMass(actorIndex);
				Speed moveSpeed = actors.move_getSpeed(actorIndex);
				lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
			}
			else
				deadMass += actors.getMass(actorIndex);
		}
	}
	assert(lowestMoveSpeed != 0);
	Mass totalMass = deadMass + (rollingMass * Config::rollingMassModifier) + (floatingMass * Config::floatingMassModifier);
	if(totalMass <= carryMass)
		return lowestMoveSpeed;
	float ratio = (float)carryMass.get() / (float)totalMass.get();
	if(ratio < Config::minimumOverloadRatio)
		return Speed::create(0);
	return Speed::create(std::ceil(lowestMoveSpeed.get() * ratio * ratio));
}