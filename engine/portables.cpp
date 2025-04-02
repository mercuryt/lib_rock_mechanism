#include "portables.hpp"


// Class method.
Speed PortablesHelpers::getMoveSpeedForGroup(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems)
{
	return getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, Mass::create(0), Mass::create(0));
}
Speed PortablesHelpers::getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, const Mass& addedRollingMass, const Mass& addedDeadMass)
{
	Mass rollingMass = addedRollingMass;
	Mass deadMass = addedDeadMass;
	Mass carryMass = Mass::create(0);
	Speed lowestMoveSpeed = Speed::create(0);
	for(ActorOrItemIndex index : actorsAndItems)
	{
		if(index.isItem())
		{
			const ItemIndex& itemIndex = ItemIndex::cast(index.get());
			Mass mass = area.getItems().getMass(itemIndex);
			static MoveTypeId roll = MoveType::byName("roll");
			if(area.getItems().getMoveType(itemIndex) == roll)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(index.isActor());
			const ActorIndex& actorIndex = ActorIndex::cast(index.get());
			const Actors& actors = area.getActors();
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
	Mass totalMass = deadMass + (rollingMass * Config::rollingMassModifier);
	if(totalMass <= carryMass)
		return lowestMoveSpeed;
	float ratio = (float)carryMass.get() / (float)totalMass.get();
	if(ratio < Config::minimumOverloadRatio)
		return Speed::create(0);
	return Speed::create(std::ceil(lowestMoveSpeed.get() * ratio * ratio));
}