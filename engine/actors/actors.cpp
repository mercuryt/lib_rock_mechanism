#include "actors.h"
#include "../animalSpecies.h"
#include "../area.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include "../eventSchedule.hpp"
#include "../items/items.h"
#include "../materialType.h"
#include "../simulation.h"
#include "../simulation/hasActors.h"
#include "../types.h"
#include "../util.h"
#include "../objectives/wait.h"
#include "../itemType.h"
#include "../sleep.h"
#include "../drink.h"
#include "../eat.h"
#include "../actors/grow.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
Percent ActorParamaters::getPercentGrown(Simulation& simulation)
{
	if(percentGrown == nullPercent)
	{
		Percent percentLifeTime = simulation.m_random.getInRange(0, 100);
		// Don't generate actors in the last 15% of their possible life span, they don't get out much.
		Step adjustedMax = util::scaleByPercent(species.deathAgeSteps[0], 85);
		// Using util::scaleByPercent and util::fractionToPercent give the wrong result here for some reason.
		Step ageSteps = ((float)adjustedMax / (float)percentLifeTime) * 100.f;
		percentGrown = std::min(100, (Percent)(((float)ageSteps / (float)species.stepsTillFullyGrown) * 100.f));
		birthStep = simulation.m_step - ageSteps;
	}
	return percentGrown;
}
std::wstring ActorParamaters::getName(Simulation& simulation)
{
	if(name.empty())
		name = util::stringToWideString(species.name) + std::to_wstring(getId(simulation));
	return name;
}
Step ActorParamaters::getBirthStep(Simulation& simulation)
{
	if(!birthStep)
	{
		if(percentGrown == nullPercent)
			getPercentGrown(simulation);
		else
		{
			// Pick an age for the given percent grown.
			Step age =  percentGrown < 95 ?
				util::scaleByPercent(species.stepsTillFullyGrown, percentGrown) :
				simulation.m_random.getInRange(species.stepsTillFullyGrown, species.deathAgeSteps[1]);
			birthStep = simulation.m_step - age;
		}
	}
	return birthStep;
}
ActorId ActorParamaters::getId(Simulation& simulation)
{
	if(!id)
		id = simulation.m_actors.getNextId();
	return id;
}
Percent ActorParamaters::getPercentThirst(Simulation& simulation)
{
	if(percentThirst == nullPercent)
	{
		percentThirst = simulation.m_random.getInRange(0, 100);
		needsDrink = simulation.m_random.percentChance(10);
	}
	return percentThirst;
}
Percent ActorParamaters::getPercentHunger(Simulation& simulation)
{
	if(percentHunger == nullPercent)
	{
		percentHunger = simulation.m_random.getInRange(0, 100);
		needsEat = simulation.m_random.percentChance(10);
	}
	return percentHunger;
}
Percent ActorParamaters::getPercentTired(Simulation& simulation)
{
	if(percentTired == nullPercent)
	{
		percentTired = simulation.m_random.getInRange(0, 100);
		needsSleep = simulation.m_random.percentChance(10);
	}
	return percentTired;
}
void ActorParamaters::generateEquipment(Area& area, ActorIndex actor)
{
	static const MaterialType& leather = MaterialType::byName("leather");
	static const MaterialType& cotton = MaterialType::byName("cotton");
	static const MaterialType& iron = MaterialType::byName("iron");
	static const MaterialType& bronze = MaterialType::byName("bronze");
	static const MaterialType& poplarWoodType = MaterialType::byName("poplar wood");
	Actors& actors = area.getActors();
	if(!actors.isSentient(actor))
		return;
	auto& random = area.m_simulation.m_random;
	auto generate = [&](const ItemType& itemType, const MaterialType& materialType){
		uint quality = random.getInRange(10, 50);
		Percent wear = random.getInRange(10, 60);
		ItemIndex item = area.getItems().create({
			.itemType=itemType,
			.materialType=materialType,
			.quality=quality,
			.percentWear=wear
		});
		actors.equipment_add(actor, item);
	};
	auto generateMetal = [&](const ItemType& itemType) {
		const MaterialType* materialType = random.chance(0.8) ? &iron : &bronze;
		generate(itemType, *materialType);
	};
	if(hasCloths)
	{
		//TODO: Cultural differences.
		static const ItemType& pantsType = ItemType::byName("pants");
		generate(pantsType, cotton);
		static const ItemType& shirtType = ItemType::byName("shirt");
		generate(shirtType, cotton);
		static const ItemType& jacketType = ItemType::byName("jacket");
		generate(jacketType, cotton);
		static const ItemType& shoesType = ItemType::byName("shoes");
		generate(shoesType, cotton);
		bool hasBelt = random.chance(0.6);
		if(hasBelt)
		{
			static const ItemType& beltType = ItemType::byName("belt");
			generate(beltType, leather);
		}
	}
	static const ItemType& halfHelmType = ItemType::byName("half helm");
	static const ItemType& breastPlateType = ItemType::byName("breast plate");
	if(hasLightArmor && !hasHeavyArmor)
	{
		if(random.chance(0.8))
		{
			generateMetal(halfHelmType);
		}
		else if(random.chance(0.5))
		{
			static const ItemType& hoodType = ItemType::byName("hood");
			generate(hoodType, leather);
		}
		if(random.chance(0.4))
		{
			generateMetal(breastPlateType);
		}
		else if(random.chance(0.6))
		{
			static const ItemType& chainMailShirtType = ItemType::byName("chain mail shirt");
			generateMetal(chainMailShirtType);
		}
	}
	if(hasHeavyArmor)
	{
		if(random.chance(0.75))
		{
			static const ItemType& fullHelmType = ItemType::byName("full helm");
			generateMetal(fullHelmType);
		}
		else if(random.chance(0.9))
		{
			generateMetal(halfHelmType);
		}
		generateMetal(breastPlateType);
		static const ItemType& greavesType = ItemType::byName("greaves");
		generateMetal(greavesType);
		static const ItemType& vambracesType = ItemType::byName("vambraces");
		generateMetal(vambracesType);
	}
	static const ItemType& longSwordType = ItemType::byName("long sword");
	if(hasSidearm)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 95)
		{
			generateMetal(longSwordType);
		}
		if(roll > 70)
		{
			static const ItemType& shortSwordType = ItemType::byName("short sword");
			generateMetal(shortSwordType);
		}
		else
		{
			static const ItemType& daggerType = ItemType::byName("dagger");
			generateMetal(daggerType);
		}
	}
	if(hasLongarm)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 75)
		{
			static const ItemType& spearType = ItemType::byName("spear");
			generateMetal(spearType);
		}
		if(roll > 65)
		{
			static const ItemType& maceType = ItemType::byName("mace");
			generateMetal(maceType);
		}
		if(roll > 45)
		{
			static const ItemType& glaveType = ItemType::byName("glave");
			generateMetal(glaveType);
		}
		if(roll > 25)
		{
			static const ItemType& axeType = ItemType::byName("axe");
			generateMetal(axeType);
		}
		else
		{
			generateMetal(longSwordType);
			static const ItemType& shieldType = ItemType::byName("shield");
			generate(shieldType, poplarWoodType);
		}
	}
	if(hasRangedWeapon)
	{
		auto roll = random.getInRange(0, 100);
		if(roll > 75)
		{
			static const ItemType& crossbowType = ItemType::byName("crossbow");
			generate(crossbowType, poplarWoodType);
		}
		else
		{
			static const ItemType& shortbowType = ItemType::byName("shortbow");
			generate(shortbowType, poplarWoodType);
		}
	}
}
Actors::Actors(Area& area) :
	Portables(area),
	m_coolDownEvent(area.m_eventSchedule),
	m_moveEvent(area.m_eventSchedule)
{ }
void Actors::resize(HasShapeIndex newSize)
{
	m_canReserve.resize(newSize);
	m_hasUniform.resize(newSize);
	m_equipmentSet.resize(newSize);
	m_id.resize(newSize);
	m_name.resize(newSize);
	m_species.resize(newSize);
	m_project.resize(newSize);
	m_birthStep.resize(newSize);
	m_causeOfDeath.resize(newSize);
	m_unencomberedCarryMass.resize(newSize);
	m_attributes.resize(newSize);
	m_hasObjectives.resize(newSize);
	m_body.resize(newSize);
	m_mustSleep.resize(newSize);
	m_mustDrink.resize(newSize);
	m_canGrow.resize(newSize);
	m_needsSafeTemperature.resize(newSize);
	m_skillSet.resize(newSize);
	m_carrying.resize(newSize);
	m_stamina.resize(newSize);
	m_canSee.resize(newSize);
	m_visionRange.resize(newSize);
	m_hasVisionFacade.resize(newSize);
	m_coolDownEvent.resize(newSize);
	m_getIntoAttackPositionPathRequest.resize(newSize);
	m_meleeAttackTable.resize(newSize);
	m_targetedBy.resize(newSize);
	m_target.resize(newSize);
	m_onMissCoolDownMelee.resize(newSize);
	m_maxMeleeRange.resize(newSize);
	m_maxRange.resize(newSize);
	m_coolDownDurationModifier.resize(newSize);
	m_combatScore.resize(newSize);
	m_moveEvent.resize(newSize);
	m_pathRequest.resize(newSize);
	m_path.resize(newSize);
	m_pathIter.resize(newSize);
	m_destination.resize(newSize);
	m_speedIndividual.resize(newSize);
	m_speedActual.resize(newSize);
	m_moveRetries.resize(newSize);
}
void Actors::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	assert(indexCanBeMoved(oldIndex));
	assert(indexCanBeMoved(newIndex));
	assert(false);
}
bool Actors::indexCanBeMoved(HasShapeIndex index) const
{
	if(!Portables::indexCanBeMoved(index))
		return false;
	if(project_exists(index))
		return false;
	if(!m_targetedBy.at(index).empty())
		return false;
	if(m_pathRequest.at(index).get() != nullptr)
		return false;
	//TODO: more checks?
	return true;
}
void Actors::onChangeAmbiantSurfaceTemperature()
{
	for(ActorIndex index : m_onSurface)
		m_needsSafeTemperature.at(index)->onChange(m_area);
}
/*
Actor::Actor(ActorParamaters params) :
	HasShape(*params.simulation, params.species.shapeForPercentGrown(params.getPercentGrown()), false, 0, 1, params.faction),
	m_attributes(params.species, params.getPercentGrown()), m_equipmentSet(*this), m_canFight(*this, *params.simulation), m_canMove(*this, *params.simulation),
	m_hasObjectives(*this), m_body(*this, *params.simulation), m_canReserve(m_faction), m_mustSleep(*this, *params.simulation),
	m_mustDrink(*this, *params.simulation), m_mustEat(*this, *params.simulation), m_canSee(*this, params.species.visionRange),
	m_canGrow(*this, params.getPercentGrown(), *params.simulation), m_needsSafeTemperature(*this, *params.simulation),
	m_hasUniform(*this), m_canPickup(*this), m_stamina(*this), m_species(params.species), m_project(nullptr),
	m_name(params.getName()), m_birthStep(params.getBirthStep()), m_causeOfDeath(CauseOfDeath::none), m_id(params.getId())
{
	m_canMove.setMoveType(m_species.moveType);
	sharedConstructor();
	scheduleNeeds();
	if(m_species.sentient)
		params.generateEquipment(*this);
	if(params.location != BLOCK_INDEX_MAX)
	{
		assert(params.area);
		setLocation(params.location, params.area);
	}
}
*/
ActorIndex Actors::create(ActorParamaters params)
{
	ActorIndex index = getNextIndex();
	bool isStatic = false;
	Portables::create(index, params.species.moveType, params.species.shapeForPercentGrown(params.getPercentGrown(m_area.m_simulation)), params.location, params.facing, isStatic);
	Simulation& s = m_area.m_simulation;
	m_id.at(index) = params.getId(s);
	m_name.at(index) = params.getName(s);
	m_species.at(index) = &params.species;
	m_project.at(index) = nullptr;
	m_birthStep.at(index) = params.getBirthStep(s);
	m_causeOfDeath.at(index) = CauseOfDeath::none;
	m_unencomberedCarryMass.at(index) = 0;
	m_attributes.at(index) = std::make_unique<Attributes>(params.species, params.getPercentGrown(s));
	m_hasObjectives.at(index) = std::make_unique<HasObjectives>(index);
	m_body.at(index) = std::make_unique<Body>(m_area, index);
	m_mustSleep.at(index) = std::make_unique<MustSleep>(m_area, index);
	m_mustDrink.at(index) = std::make_unique<MustDrink>(m_area, index);
	m_canGrow.at(index) = std::make_unique<CanGrow>(m_area, index, params.getPercentGrown(s));
	m_needsSafeTemperature.at(index) = std::make_unique<ActorNeedsSafeTemperature>(m_area, index);
	m_skillSet.at(index) = std::make_unique<SkillSet>();
	// CanPickUp.
	m_carrying.at(index).clear();
	// Stamina.
	m_stamina.at(index) = 0;
	// Vision.
	assert(m_canSee.at(index).empty());
	m_visionRange.at(index) = params.species.visionRange;
	assert(m_hasVisionFacade.at(index).empty());
	// Combat.
	assert(!m_coolDownEvent.exists(index));
	assert(m_meleeAttackTable.at(index).empty());
	assert(m_targetedBy.at(index).empty());
	m_target.at(index) = ACTOR_INDEX_MAX;
	m_onMissCoolDownMelee.at(index) = 0;
	m_maxMeleeRange.at(index) = 0;
	m_maxRange.at(index) = 0;
	m_coolDownDurationModifier.at(index) = 0;
	m_combatScore.at(index) = 0;
	// Move.
	assert(!m_moveEvent.exists(index));
	assert(m_pathRequest.at(index) == nullptr);
	assert(m_path.at(index).empty());
	m_destination.at(index) = BLOCK_INDEX_MAX;
	m_speedIndividual.at(index) = 0;
	m_speedActual.at(index) = 0;
	m_moveRetries.at(index) = 0;
	s.m_actors.registerActor(m_id.at(index), *this, index);
	sharedConstructor(index);
	scheduleNeeds(index);
	if(isSentient(index))
		params.generateEquipment(m_area, index);
	if(params.location != BLOCK_INDEX_MAX)
		setLocationAndFacing(index, params.location, params.facing);
	return index;
}
void Actors::sharedConstructor(ActorIndex index)
{
	combat_update(index);
	move_updateIndividualSpeed(index);
	m_body.at(index)->initalize(m_area);
	m_mustDrink.at(index)->setFluidType(m_species.at(index)->fluidType);
}
void Actors::scheduleNeeds(ActorIndex index)
{
	m_mustSleep.at(index)->scheduleTiredEvent(m_area);
	m_mustDrink.at(index)->scheduleDrinkEvent(m_area);
	m_mustEat.at(index)->scheduleHungerEvent(m_area);
	m_canGrow.at(index)->updateGrowingStatus(m_area);
}
void Actors::setLocation(ActorIndex index, BlockIndex block)
{
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	Facing facing = m_area.getBlocks().facingToSetWhenEnteringFrom(block, m_location.at(index));
	setLocationAndFacing(index, block, facing);
}
void Actors::setLocationAndFacing(ActorIndex index, BlockIndex block, Facing facing)
{
	if(m_location.at(index) != BLOCK_INDEX_MAX)
		exit(index);
	Blocks& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : m_shape.at(index)->makeOccupiedPositionsWithFacing(facing))
	{
		BlockIndex occupied = blocks.offset(block, x, y, z);
		blocks.actor_record(occupied, index, v);
	}
}
void Actors::exit(ActorIndex index)
{
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	BlockIndex location = m_location.at(index);
	auto& blocks = m_area.getBlocks();
	for(auto [x, y, z, v] : m_shape.at(index)->makeOccupiedPositionsWithFacing(m_facing.at(index)))
	{
		BlockIndex occupied = blocks.offset(location, x, y, z);
		blocks.actor_erase(occupied, index);
	}
	m_location.at(index) = BLOCK_INDEX_MAX;
}
void Actors::resetNeeds(ActorIndex index)
{
	m_mustSleep.at(index)->notTired(m_area);
	m_mustDrink.at(index)->notThirsty(m_area);
	m_mustEat.at(index)->notHungry(m_area);
}
void Actors::removeMassFromCorpse(ActorIndex index, Mass mass)
{
	assert(!isAlive(index));
	assert(mass <= m_attributes.at(index)->getMass());
	m_attributes.at(index)->removeMass(mass);
}
bool Actors::isEnemy(ActorIndex index, ActorIndex other) const { return m_faction.at(index)->enemies.contains(m_faction.at(other)); }
bool Actors::isAlly(ActorIndex index, ActorIndex other) const { return m_faction.at(index)->allies.contains(m_faction.at(other)); }
void Actors::die(ActorIndex index, CauseOfDeath causeOfDeath)
{
	m_causeOfDeath.at(index) = causeOfDeath;
	combat_onDeath(index);
	move_onDeath(index);
	m_mustDrink.at(index)->onDeath();
	m_mustEat.at(index)->onDeath();
	m_mustSleep.at(index)->onDeath();
	if(m_project.at(index) != nullptr)
		m_project.at(index)->removeWorker(index);
	if(m_location.at(index) != BLOCK_INDEX_MAX)
		setStatic(index, true);
	m_area.m_visionFacadeBuckets.remove(index);
}
void Actors::passout(ActorIndex, Step)
{
	//TODO
}
void Actors::leaveArea(ActorIndex index)
{
	combat_onLeaveArea(index);
	move_onLeaveArea(index);
	exit(index);
}
void Actors::wait(ActorIndex index, Step duration)
{
	m_hasObjectives.at(index)->addTaskToStart(m_area, std::make_unique<WaitObjective>(m_area, index, duration));
}
void Actors::takeHit(ActorIndex index, Hit& hit, BodyPart& bodyPart)
{
	m_equipmentSet.at(index)->modifyImpact(m_area, hit, bodyPart.bodyPartType);
	m_body.at(index)->getHitDepth(hit, bodyPart);
	if(hit.depth != 0)
		m_body.at(index)->addWound(m_area, bodyPart, hit);
}
void Actors::setFaction(ActorIndex index, Faction* faction)
{
	m_faction.at(index) = faction;
	if(faction == nullptr)
		m_canReserve.at(index) = nullptr;
	else
		m_canReserve.at(index)->setFaction(*faction);
}
Mass Actors::getMass(ActorIndex index) const
{
	return m_attributes.at(index)->getMass() + m_equipmentSet.at(index)->getMass() + canPickUp_getMass(index);
}
Volume Actors::getVolume(ActorIndex index) const
{
	return m_body.at(index)->getVolume(m_area);
}
Quantity Actors::getAgeInYears(ActorIndex index) const
{
	DateTime now(m_area.m_simulation.m_step);
	DateTime birthDate(m_birthStep.at(index));
	Quantity differenceYears = now.year - birthDate.year;
	if(now.day > birthDate.day)
		++differenceYears;
	return differenceYears;
}
Step Actors::getAge(ActorIndex index) const
{
	return m_area.m_simulation.m_step - m_birthStep.at(index);
}
std::wstring Actors::getActionDescription(ActorIndex index) const
{
	if(m_hasObjectives.at(index)->hasCurrent())
		return util::stringToWideString(const_cast<HasObjectives&>(*m_hasObjectives.at(index).get()).getCurrent().name());
	return L"no action";
}
void Actors::reserveAllBlocksAtLocationAndFacing(ActorIndex index, const BlockIndex location, Facing facing)
{
	if(m_faction.at(index) == nullptr)
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().reserve(occupied, *m_canReserve.at(index));
}
void Actors::unreserveAllBlocksAtLocationAndFacing(ActorIndex index, const BlockIndex location, Facing facing)
{
	if(m_faction.at(index) == nullptr)
		return;
	for(BlockIndex occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		m_area.getBlocks().unreserve(occupied, *m_canReserve.at(index));
}
void Actors::setBirthStep(ActorIndex index, Step step)
{
	m_birthStep.at(index) = step;
	m_canGrow.at(index)->updateGrowingStatus(m_area);
}
Percent Actors::getPercentGrown(ActorIndex index) const { return m_canGrow.at(index)->growthPercent(); }
void Actors::log(ActorIndex index) const
{
	std::wcout << m_name.at(index);
	std::cout << "(" << m_species.at(index)->name << ")";
	Portables::log(index);
	std::cout << std::endl;
}
void Actors::satisfyNeeds(ActorIndex index)
{
	// Wake up if asleep.
	if(!m_mustSleep.at(index)->isAwake())
		m_mustSleep.at(index)->wakeUp(m_area);
	else
		m_mustSleep.at(index)->notTired(m_area);
	// Discard drink objective if exists.
	if(m_mustDrink.at(index)->getVolumeFluidRequested() != 0)
		m_mustDrink.at(index)->drink(m_area, m_mustDrink.at(index)->getVolumeFluidRequested());
	// Discard eat objective if exists.
	if(m_mustEat.at(index)->getMassFoodRequested() != 0)
		m_mustEat.at(index)->eat(m_area, m_mustEat.at(index)->getMassFoodRequested());
}
// Sleep.
void Actors::sleep_do(ActorIndex index) { m_mustSleep.at(index)->sleep(m_area); }
void Actors::sleep_wakeUp(ActorIndex index){ m_mustSleep.at(index)->wakeUp(m_area); }
void Actors::sleep_wakeUpEarly(ActorIndex index){ m_mustSleep.at(index)->wakeUpEarly(m_area); }
void Actors::sleep_setSpot(ActorIndex index, BlockIndex location) { m_mustSleep.at(index)->setLocation(location); }
void Actors::sleep_makeTired(ActorIndex index) { m_mustSleep.at(index)->tired(m_area); }
bool Actors::sleep_isAwake(ActorIndex index) const { return m_mustSleep.at(index)->isAwake(); }
Percent Actors::sleep_getPercentDoneSleeping(ActorIndex index) const { return m_mustSleep.at(index)->getSleepPercent(); }
Percent Actors::sleep_getPercentTired(ActorIndex index) const { return m_mustSleep.at(index)->getTiredPercent(); }
BlockIndex Actors::sleep_getSpot(ActorIndex index) const { return m_mustSleep.at(index)->getLocation(); }
bool Actors::sleep_hasTiredEvent(ActorIndex index) const { return m_mustSleep.at(index)->hasTiredEvent(); }
// Attributes.
uint32_t Actors::getStrength(ActorIndex index) const { return m_attributes.at(index)->getStrength(); }
uint32_t Actors::getDextarity(ActorIndex index) const { return m_attributes.at(index)->getDextarity(); }
uint32_t Actors::getAgility(ActorIndex index) const { return m_attributes.at(index)->getAgility(); }
// Skills.
uint32_t Actors::skill_getLevel(ActorIndex index, const SkillType& skillType) const { return m_skillSet.at(index)->get(skillType); }
// CoolDownEvent.
void AttackCoolDownEvent::execute(Simulation&, Area* area) { area->getActors().combat_coolDownCompleted(m_index); }
void AttackCoolDownEvent::clearReferences(Simulation&, Area* area) { area->getActors().m_coolDownEvent.clearPointer(m_index); }
