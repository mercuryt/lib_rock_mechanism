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
	static MoveTypeId roll = MoveType::byName("roll");
	static MoveTypeId floating = MoveType::byName("floating");
	auto recordMass = [&](const MoveTypeId& moveType, const Mass& mass) {

				if(moveType == floating)
					floatingMass += mass;
				else if(moveType == roll)
					rollingMass += mass;
				else
					deadMass += mass;
	};
	for(ActorOrItemIndex index : actorsAndItems)
	{
		if(index.isItem())
		{
			const ItemIndex& itemIndex = ItemIndex::cast(index.get());
			const MoveTypeId& moveType = items.getMoveType(itemIndex);
			const ActorIndex& pilot = items.pilot_get(itemIndex);
			Mass mass = items.getMass(itemIndex);
			if(pilot.exists())
			{
				Speed moveSpeed = actors.move_getSpeed(pilot);
				// A vehicle which is pulled by actors has no intrinsic move speed and thus doesn't contribute to calculating lowestMoveSpeed.
				if(moveSpeed != 0)
				{
					lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
					carryMass += mass * Config::vehicleMassToCarryMassModifier;
				}
				else
					recordMass(moveType, mass);
			}
			else
				recordMass(moveType, mass);
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
			{
				const MoveTypeId& moveType = actors.getMoveType(actorIndex);
				recordMass(moveType, actors.getMass(actorIndex));
			}
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