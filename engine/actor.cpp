#include "actor.h"
#include "animalSpecies.h"
#include "block.h"
#include "area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "item.h"
#include "materialType.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include "wait.h"

#include <algorithm>
#include <cstddef>
#include <iostream>
Percent ActorParamaters::getPercentGrown()
{
	if(percentGrown == nullPercent)
	{
		Percent percentLifeTime = simulation->m_random.getInRange(0, 100);
		// Don't generate actors in the last 15% of their possible life span, they don't get out much.
		Step adjustedMax = util::scaleByPercent(species.deathAgeSteps[0], 85);
		// Using util::scaleByPercent and util::fractionToPercent give the wrong result here for some reason.
		Step ageSteps = ((float)adjustedMax / (float)percentLifeTime) * 100.f;
		percentGrown = std::min(100, (Percent)(((float)ageSteps / (float)species.stepsTillFullyGrown) * 100.f));
		birthStep = simulation->m_step - ageSteps;
	}
	return percentGrown;
}

std::wstring ActorParamaters::getName()
{
	if(name.empty())
		name = util::stringToWideString(species.name) + std::to_wstring(getId());
	return name;
}
Step ActorParamaters::getBirthStep()
{
	if(!birthStep)
	{
		if(percentGrown == nullPercent)
			getPercentGrown();
		else
		{
			// Pick an age for the given percent grown.
			Step age =  percentGrown < 95 ?
				util::scaleByPercent(species.stepsTillFullyGrown, percentGrown) :
				simulation->m_random.getInRange(species.stepsTillFullyGrown, species.deathAgeSteps[1]);
			birthStep = simulation->m_step - age;
		}
	}
	return birthStep;
}
ActorId ActorParamaters::getId()
{
	if(!id)
		id = simulation->m_nextActorId++;
	return id;
}
Percent ActorParamaters::getPercentThirst()
{
	if(percentThirst == nullPercent)
	{
		percentThirst = simulation->m_random.getInRange(0, 100);
		needsDrink = simulation->m_random.percentChance(10);
	}
	return percentThirst;
}
Percent ActorParamaters::getPercentHunger()
{
	if(percentHunger == nullPercent)
	{
		percentHunger = simulation->m_random.getInRange(0, 100);
		needsEat = simulation->m_random.percentChance(10);
	}
	return percentHunger;
}
Percent ActorParamaters::getPercentTired()
{
	if(percentTired == nullPercent)
	{
		percentTired = simulation->m_random.getInRange(0, 100);
		needsSleep = simulation->m_random.percentChance(10);
	}
	return percentTired;
}
void ActorParamaters::generateEquipment(Actor& actor)
{
	static const MaterialType& leather = MaterialType::byName("leather");
	static const MaterialType& cotton = MaterialType::byName("cotton");
	static const MaterialType& iron = MaterialType::byName("iron");
	static const MaterialType& bronze = MaterialType::byName("bronze");
	static const MaterialType& poplarWoodType = MaterialType::byName("poplar wood");
	if(!actor.isSentient())
		return;
	auto& random = simulation->m_random;
	auto generate = [&](const ItemType& itemType, const MaterialType& materialType){
		Percent quality = random.getInRange(10, 50);
		Percent wear = random.getInRange(10, 60);
		Item& item = simulation->createItemNongeneric(itemType, materialType, quality, wear);
		actor.m_equipmentSet.addEquipment(item);
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
Actor::Actor(Simulation& simulation, uint32_t id, const std::wstring& name, const AnimalSpecies& species, Step birthStep, Percent percentGrown, Faction* faction, Attributes attributes) :
	HasShape(simulation, species.shapeForPercentGrown(percentGrown), false), m_faction(faction), m_birthStep(birthStep), m_id(id), m_name(name), m_species(species), m_alive(true), m_body(*this), m_project(nullptr), m_attributes(attributes), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_needsSafeTemperature(*this), m_canPickup(*this), m_equipmentSet(*this), m_canMove(*this), m_canFight(*this), m_canGrow(*this, percentGrown), m_hasObjectives(*this), m_canReserve(faction), m_stamina(*this), m_hasUniform(*this), m_visionRange(species.visionRange)
{
	// TODO: Having this line here requires making the existance of objectives mandatory at all times. Good idea?
	//m_hasObjectives.getNext();
}
Actor::Actor(ActorParamaters params) : HasShape(*params.simulation, params.species.shapeForPercentGrown(params.getPercentGrown()), false),
	m_faction(params.faction), m_birthStep(params.getBirthStep()), m_id(params.getId()), m_name(params.getName()),
	m_species(params.species), m_alive(true), m_body(*this), m_project(nullptr), 
	m_attributes(m_species, params.getPercentGrown()), m_mustEat(*this), m_mustDrink(*this), m_mustSleep(*this), m_needsSafeTemperature(*this), 
	m_canPickup(*this), m_equipmentSet(*this), m_canMove(*this), m_canFight(*this), m_canGrow(*this, params.getPercentGrown()), 
	m_hasObjectives(*this), m_canReserve(m_faction), m_stamina(*this), m_hasUniform(*this), m_visionRange(m_species.visionRange) 
	{ 
		if(params.location)
		{
			setLocation(*params.location);
			m_location->m_area->m_hasActors.add(*this);
		}
		if(m_species.sentient)
			params.generateEquipment(*this);
	}
Actor::Actor(const Json& data, DeserializationMemo& deserializationMemo) :
	HasShape(data, deserializationMemo),
	m_faction(data.contains("faction") ? &deserializationMemo.m_simulation.m_hasFactions.byName(data["faction"].get<std::wstring>()) : nullptr), 
	m_birthStep(data["birthStep"]), m_id(data["id"].get<ActorId>()), m_name(data["name"].get<std::wstring>()), 
	m_species(AnimalSpecies::byName(data["species"].get<std::string>())), m_alive(data["alive"].get<bool>()), 
	m_body(data["body"], deserializationMemo, *this), m_project(nullptr), 
	m_attributes(data["attributes"], m_species, data["percentGrown"].get<Percent>()), 
	m_mustEat(data["mustEat"], *this), m_mustDrink(data["mustDrink"], *this), m_mustSleep(data["mustSleep"], *this), 
	m_needsSafeTemperature(data["needsSafeTemperature"], *this), m_canPickup(data["canPickup"], *this), 
	m_equipmentSet(data["equipmentSet"], *this), m_canMove(data["canMove"], deserializationMemo, *this), m_canFight(data["canFight"], *this), 
	m_canGrow(data["canGrow"], *this), 
	// Wait untill projects have been loaded before loading hasObjectives.
	m_hasObjectives(*this), m_canReserve(m_faction), m_stamina(*this, data["stamina"].get<uint32_t>()), m_hasUniform(*this), 
	m_visionRange(m_species.visionRange)
{ 
	if(data.contains("skills"))
		m_skillSet.load(data["skills"]);
	if(data.contains("location"))
	{
		setLocation(deserializationMemo.blockReference(data["location"]));
		m_location->m_area->m_hasActors.add(*this);
	}
}
Json Actor::toJson() const
{
	Json data = HasShape::toJson();
	data["species"] = m_species;
	data["birthStep"] = m_birthStep;
	data["percentGrown"] = m_canGrow.growthPercent();
	data["id"] = m_id;
	data["name"] = m_name;
	data["alive"] = m_alive;
	data["body"] = m_body.toJson();
	data["attributes"] = m_attributes.toJson();
	data["equipmentSet"] = m_equipmentSet.toJson();
	data["mustEat"] = m_mustEat.toJson();
	data["mustDrink"] = m_mustDrink.toJson();
	data["mustSleep"] = m_mustSleep.toJson();
	data["canPickup"] = m_canPickup.toJson();
	data["canMove"] = m_canMove.toJson();
	data["canFight"] = m_canFight.toJson();
	data["canGrow"] = m_canGrow.toJson();
	data["needsSafeTemperature"] = m_needsSafeTemperature.toJson();
	data["stamina"] = m_stamina.get();
	data["hasObjectives"] = m_hasObjectives.toJson();
	if(m_faction != nullptr)
		data["faction"] = m_faction;
	if(m_location != nullptr)
		data["location"] = m_location;
	if(m_canReserve.hasReservations())
		data["canReserve"] = m_canReserve.toJson();
	if(m_hasUniform.exists())
		data["uniform"] = &m_hasUniform.get();
	if(!m_skillSet.m_skills.empty())
		data["skills"] = m_skillSet.toJson();
	return data;
}
void Actor::setLocation(Block& block)
{
	assert(&block != m_location);
	assert(block.m_hasShapes.anythingCanEnterEver());
	if(m_location == nullptr)
	{
		assert(block.m_hasShapes.canEnterEverWithFacing(*this, m_facing));
		bool can = block.m_hasShapes.canEnterCurrentlyWithFacing(*this, m_facing);
		assert(can);
	}
	else
	{
		assert(block.m_hasShapes.canEnterEverFrom(*this, *m_location));
		assert(block.m_hasShapes.canEnterCurrentlyFrom(*this, *m_location));
	}
	block.m_hasActors.enter(*this);
	m_canLead.onMove();
}
void Actor::exit()
{
	assert(m_location != nullptr);
	m_location->m_hasActors.exit(*this);
}

void Actor::removeMassFromCorpse(Mass mass)
{
	assert(!m_alive);
	assert(mass <= m_attributes.getMass());
	m_attributes.removeMass(mass);
}
bool Actor::isEnemy(Actor& actor) const { return m_faction->enemies.contains(const_cast<Faction*>(actor.m_faction)); }
bool Actor::isAlly(Actor& actor) const { return m_faction == actor.m_faction || m_faction->allies.contains(const_cast<Faction*>(actor.m_faction)); }
void Actor::die(CauseOfDeath causeOfDeath)
{
	m_causeOfDeath = causeOfDeath;
	m_alive = false;
	m_canFight.onDeath();
	m_canMove.onDeath();
	m_mustDrink.onDeath();
	m_mustEat.onDeath();
	m_mustSleep.onDeath();
	if(m_project != nullptr)
		m_project->removeWorker(*this);
	if(m_location)
		setStatic(true);
}
void Actor::passout(Step duration)
{
	//TODO
	(void)duration;
}
void Actor::doVision(std::unordered_set<Actor*>& actors)
{
	m_canSee.swap(actors);
	//TODO: psycology.
	//TODO: fog of war?
}
void Actor::leaveArea()
{
	m_canFight.onLeaveArea();
	m_canMove.onLeaveArea();
	Area& area = *m_location->m_area;
	// Run remove before exit becuse we need to use location to know which bucket to remove from.
	area.m_hasActors.remove(*this);
	exit();
}
void Actor::wait(Step duration)
{
	m_hasObjectives.addTaskToStart(std::make_unique<WaitObjective>(*this, duration));
}
void Actor::takeHit(Hit& hit, BodyPart& bodyPart)
{
	m_equipmentSet.modifyImpact(hit, bodyPart.bodyPartType);
	m_body.getHitDepth(hit, bodyPart);
	if(hit.depth != 0)
		m_body.addWound(bodyPart, hit);
}
void Actor::setFaction(const Faction* faction) 
{ 
	m_faction = faction; 
	m_canReserve.setFaction(faction); 
}
Mass Actor::getMass() const
{
	return m_attributes.getMass() + m_equipmentSet.getMass() + m_canPickup.getMass();
}
Volume Actor::getVolume() const
{
	return m_body.getVolume();
}
uint32_t Actor::getAgeInYears() const
{
	DateTime now(const_cast<Actor&>(*this).getSimulation().m_step);
	DateTime birthDate(m_birthStep);
	uint32_t differenceYears = now.year - birthDate.year;
	if(now.day > birthDate.day)
		++differenceYears;
	return differenceYears;
}
Step Actor::getAge() const
{
	return const_cast<Actor*>(this)->getSimulation().m_step - m_birthStep;
}
bool Actor::allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing) const
{
	if(m_faction == nullptr)
		return true;
	return HasShape::allBlocksAtLocationAndFacingAreReservable(location, facing, *m_faction);
}
void Actor::reserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing)
{
	if(m_faction == nullptr)
		return;
	for(Block* occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
		occupied->m_reservable.reserveFor(m_canReserve, 1u);
}
void Actor::unreserveAllBlocksAtLocationAndFacing(const Block& location, Facing facing)
{
	if(m_faction == nullptr)
		return;
	for(Block* occupied : getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
		occupied->m_reservable.clearReservationFor(m_canReserve);
}
void Actor::setBirthStep(Step step)
{
	m_birthStep = step;
	m_canGrow.updateGrowingStatus();
}
EventSchedule& Actor::getEventSchedule() { return getSimulation().m_eventSchedule; }
ThreadedTaskEngine& Actor::getThreadedTaskEngine() { return getSimulation().m_threadedTaskEngine; }
void Actor::log() const
{
	std::wcout << m_name;
	std::cout << "(" << m_species.name << ")";
	HasShape::log();
	std::cout << std::endl;
}
void Actor::satisfyNeeds()
{
	// Wake up if asleep.
	if(!m_mustSleep.isAwake())
		m_mustSleep.wakeUp();
	else
		m_mustSleep.notTired();
	// Discard drink objective if exists.
	if(m_mustDrink.getVolumeFluidRequested() != 0)
		m_mustDrink.drink(m_mustDrink.getVolumeFluidRequested());
	// Discard eat objective if exists.
	if(m_mustEat.getMassFoodRequested() != 0)
		m_mustEat.eat(m_mustEat.getMassFoodRequested());
}
// To be used by block.
void BlockHasActors::enter(Actor& actor)
{
	assert(!contains(actor));
	if(actor.m_location != nullptr)
	{
		actor.m_facing = m_block.facingToSetWhenEnteringFrom(*actor.m_location);
		m_block.m_area->m_hasActors.m_locationBuckets.update(actor, *actor.m_location, m_block);
		actor.m_location->m_hasActors.exit(actor);
	}
	else
		m_block.m_area->m_hasActors.m_locationBuckets.insert(actor, m_block);
	m_actors.push_back(&actor);
	m_block.m_hasShapes.enter(actor);
	if(m_block.m_underground)
		m_block.m_area->m_hasActors.setUnderground(actor);
	else
		m_block.m_area->m_hasActors.setNotUnderground(actor);
}
void BlockHasActors::exit(Actor& actor)
{
	assert(contains(actor));
	std::erase(m_actors, &actor);
	m_block.m_hasShapes.exit(actor);
}
void BlockHasActors::setTemperature(Temperature temperature)
{
	(void)temperature;
	for(Actor* actor : m_actors)
		actor->m_needsSafeTemperature.onChange();
}
bool BlockHasActors::contains(Actor& actor) const
{
	return std::ranges::find(m_actors, &actor) != m_actors.end();
}
void AreaHasActors::add(Actor& actor)
{
	assert(actor.m_location != nullptr);
	assert(!m_actors.contains(&actor));
	m_actors.insert(&actor);
	m_locationBuckets.insert(actor, *actor.m_location);
	m_visionBuckets.add(actor);
	if(!actor.m_location->m_underground)
		m_onSurface.insert(&actor);
}
void AreaHasActors::remove(Actor& actor)
{
	m_actors.erase(&actor);
	m_locationBuckets.erase(actor);
	m_visionBuckets.remove(actor);
	m_onSurface.erase(&actor);
}
void AreaHasActors::processVisionReadStep()
{
	m_visionRequestQueue.clear();
	for(Actor* actor : m_visionBuckets.get(m_area.m_simulation.m_step))
		m_visionRequestQueue.emplace_back(*actor);
	auto visionIter = m_visionRequestQueue.begin();
	while(visionIter < m_visionRequestQueue.end())
	{
		auto end = m_visionRequestQueue.size() <= (uintptr_t)(m_visionRequestQueue.begin() - visionIter) + Config::visionThreadingBatchSize ?
			m_visionRequestQueue.end() :
			visionIter + Config::visionThreadingBatchSize;
		m_area.m_simulation.m_taskFutures.push_back(m_area.m_simulation.m_pool.submit([=](){ VisionRequest::readSteps(visionIter, end); }));
		visionIter = end;
	}
}
void AreaHasActors::processVisionWriteStep()
{
	for(VisionRequest& visionRequest : m_visionRequestQueue)
		visionRequest.writeStep();
}
void AreaHasActors::onChangeAmbiantSurfaceTemperature()
{
	for(Actor* actor : m_onSurface)
		actor->m_needsSafeTemperature.onChange();
}
void AreaHasActors::setUnderground(Actor& actor)
{
	m_onSurface.erase(&actor);
	actor.m_needsSafeTemperature.onChange();
}
void AreaHasActors::setNotUnderground(Actor& actor)
{
	m_onSurface.insert(&actor);
	actor.m_needsSafeTemperature.onChange();
}
