#include "../engine/simulation/simulation.h"
#include "../engine/definitions/definitions.h"
#include "../engine/config.h"
#include "../engine/datetime.h"
#include "../engine/types.h"
#include "../engine/actor.h"
#include "../engine/definitions/animalSpecies.h"
#include "../engine/attributes.h"
#include "../engine/body.h"
#include "../engine/equipment.h"
#include "../engine/objective.h"
#include "../engine/plant.h"
#include "../engine/definitions/woundType.h"

#include <nanobind/nanobind.h>
#include <cstdint>

namespace nb = nanobind;

void loadConfigAndDefinitions()
{
	Config::load();
	definitions::load();
	ObjectiveType::load();
}

NB_MODULE(bound, m)
{
	m.def("loadConfigAndDefinitions", &loadConfigAndDefinitions);
	// MaterialType
	nb::class_<MaterialType>(m, "MaterialType")
		.def_ro("name", &MaterialType::name)
		.def_ro("density", &MaterialType::density)
		.def_ro("hardness", &MaterialType::hardness)
		.def_ro("meltingPoint", &MaterialType::meltingPoint)
		.def_ro("transparent", &MaterialType::transparent);
	// FluidType
	nb::class_<FluidType>(m, "FluidType")
		.def_ro("name", &FluidType::name)
		.def_ro("density", &FluidType::density);
	// ItemType
	nb::class_<ItemType>(m, "ItemType")
		.def_ro("name", &ItemType::name)
		.def_ro("installable", &ItemType::installable)
		.def_ro("volume", &ItemType::volume)
		.def_ro("generic", &ItemType::generic)
		.def_ro("internalVolume", &ItemType::internalVolume)
		.def_ro("canHoldFluids", &ItemType::canHoldFluids)
		.def_ro("value", &ItemType::value)
		.def_ro("edibleForDrinkersOf", &ItemType::edibleForDrinkersOf);
		//TODO: Weapon data.
	// PlantSpecies
	nb::class_<PlantSpecies>(m, "PlantSpecies")
		.def_ro("name", &PlantSpecies::name)
		.def_ro("annual", &PlantSpecies::annual)
		.def_ro("maximumGrowingTemperature", &PlantSpecies::maximumGrowingTemperature)
		.def_ro("minimumGrowingTemperature", &PlantSpecies::minimumGrowingTemperature)
		.def_ro("stepsTillDieFromTemperature", &PlantSpecies::stepsTillDieFromTemperature)
		.def_ro("stepsNeedsFluidFrequency", &PlantSpecies::stepsNeedsFluidFrequency)
		.def_ro("volumeFluidConsumed", &PlantSpecies::volumeFluidConsumed)
		.def_ro("stepsTillDieWithoutFluid", &PlantSpecies::stepsTillDieWithoutFluid)
		.def_ro("stepsTillFullyGrown", &PlantSpecies::stepsTillFullyGrown)
		.def_ro("stepsTillFoliageGrowsFromZero", &PlantSpecies::stepsTillFoliageGrowsFromZero)
		.def_ro("growsInSunLight", &PlantSpecies::growsInSunLight)
		.def_ro("rootRangeMax", &PlantSpecies::rootRangeMax)
		.def_ro("rootRangeMin", &PlantSpecies::rootRangeMin)
		.def_ro("adultMass", &PlantSpecies::adultMass)
		.def_ro("dayOfYearForSowStart", &PlantSpecies::dayOfYearForSowStart)
		.def_ro("dayOfYearForSowEnd", &PlantSpecies::dayOfYearForSowEnd)
		.def("getFluidTypeName", [](const PlantSpecies& species){ return species.fluidType.name; });
	// AnimalSpecies
	nb::class_<AnimalSpecies>(m, "AnimalSpecies")
		.def_ro("name", &AnimalSpecies::name)
		.def_ro("sentient", &AnimalSpecies::sentient)
	// Min, max, newborn.
		.def_ro("strength", &AnimalSpecies::strength)
		.def_ro("dextarity", &AnimalSpecies::dextarity)
		.def_ro("agility", &AnimalSpecies::agility)
		.def_ro("mass", &AnimalSpecies::mass)
		.def_ro("deathAgeSteps", &AnimalSpecies::deathAgeSteps)
		.def_ro("stepsTillFullyGrown", &AnimalSpecies::stepsTillFullyGrown)
		.def_ro("stepsTillDieWithoutFood", &AnimalSpecies::stepsTillDieWithoutFood)
		.def_ro("stepsEatFrequency", &AnimalSpecies::stepsEatFrequency)
		.def_ro("stepsTillDieWithoutFluid", &AnimalSpecies::stepsTillDieWithoutFluid)
		.def_ro("stepsFluidDrinkFreqency", &AnimalSpecies::stepsFluidDrinkFreqency)
		.def_ro("stepsTillDieInUnsafeTemperature", &AnimalSpecies::stepsTillDieInUnsafeTemperature)
		.def_ro("minimumSafeTemperature", &AnimalSpecies::minimumSafeTemperature)
		.def_ro("maximumSafeTemperature", &AnimalSpecies::maximumSafeTemperature)
		.def_ro("stepsSleepFrequency", &AnimalSpecies::stepsSleepFrequency)
		.def_ro("stepsTillSleepOveride", &AnimalSpecies::stepsTillSleepOveride)
		.def_ro("stepsSleepDuration", &AnimalSpecies::stepsSleepDuration)
		.def_ro("nocturnal", &AnimalSpecies::nocturnal)
		.def_ro("eatsMeat", &AnimalSpecies::eatsMeat)
		.def_ro("eatsLeaves", &AnimalSpecies::eatsLeaves)
		.def_ro("eatsFruit", &AnimalSpecies::eatsFruit)
		.def_ro("visionRange", &AnimalSpecies::visionRange)
		.def_ro("bodyScale", &AnimalSpecies::bodyScale)
		.def("getFluidTypeName", [](const AnimalSpecies& species){ return species.fluidType.name; })
		.def("getMaterialTypeName", [](const AnimalSpecies& species){ return species.materialType.name; })
		.def("getMoveTypeName", [](const AnimalSpecies& species){ return species.moveType.name; })
		.def("getBodyTypeName", [](const AnimalSpecies& species){ return species.bodyType.name; });
	// WoundType
	nb::enum_<WoundType>(m, "WoundType")
		.value("Pierce", WoundType::Pierce)
		.value("Cut", WoundType::Cut)
		.value("Bludgeon", WoundType::Bludgeon)
		.export_values();
	// BodyPartType
	nb::class_<BodyPartType>(m, "BodyPartType")
		.def_ro("name", &BodyPartType::name)
		.def_ro("volume", &BodyPartType::volume)
		.def_ro("doesLocamotion", &BodyPartType::doesLocamotion)
		.def_ro("doesManipulation", &BodyPartType::doesManipulation)
		.def_ro("vital", &BodyPartType::vital);
		//TODO: attackTypesAndMaterials
	// DateTime
	nb::class_<DateTime>(m, "DateTime")
		.def(nb::init<uint8_t, uint16_t, uint16_t>())
		.def_rw("hour", &DateTime::hour)
		.def_rw("day", &DateTime::day)
		.def_rw("year", &DateTime::year);
	// Cuboid
	nb::class_<Cuboid>(m, "Cuboid")
		.def(nb::init<BlockIndex&, BlockIndex&>());
	// Simulation
	nb::class_<Simulation>(m, "Simulation")
		.def(nb::init<DateTime, Step>())
		.def("doStep", &Simulation::doStep)
		.def("createArea", &Simulation::createArea, nb::rv_policy::reference)
		.def("createActor", &Simulation::createActor, nb::rv_policy::reference)
		.def("createItemGeneric", &Simulation::createItemGeneric, nb::rv_policy::reference)
		.def("createItemNongeneric", &Simulation::createItemNongeneric, nb::rv_policy::reference)
		.def_ro("m_now", &Simulation::m_now);
	// Area
	nb::class_<Area>(m, "Area")
		.def("getBlock", &Area::getBlock, nb::rv_policy::reference)
		.def("getZLevel", &Area::getZLevel, nb::rv_policy::take_ownership)
		.def_ro("id", &Area::m_id)
		.def_ro("sizeY", &Area::m_sizeY)
		.def_ro("sizeZ", &Area::m_sizeZ)
		.def("isRaining", [](Area& area){ return area.m_hasRain.isRaining(); })
		.def("rainFluidType", [](Area& area){ return area.m_hasRain.getFluidType(); })
		.def("rainIntensityPercent", [](Area& area){ return area.m_hasRain.getIntensityPercent(); });
	// BlockIndex
	nb::class_<BlockIndex>(m, "BlockIndex")
		.def("isSolid", &BlockIndex::isSolid)
		.def("getSolidMaterial", &BlockIndex::getSolidMaterial, nb::rv_policy::reference)
		.def_ro("x", &BlockIndex::m_x)
		.def_ro("y", &BlockIndex::m_y)
		.def_ro("z", &BlockIndex::m_z)
		.def_ro("exposedToSky", &BlockIndex::m_exposedToSky)
		.def_ro("underground", &BlockIndex::m_underground)
		.def_ro("outdoors", &BlockIndex::m_outdoors)
		.def_ro("totalFluidVolume", &BlockIndex::m_totalFluidVolume)
		.def("getActors", [](BlockIndex& block){ return block.m_hasActors.getAll(); }, nb::rv_policy::reference)
		.def("getItems", [](BlockIndex& block){ return block.m_hasItems.getAll(); }, nb::rv_policy::reference)
		.def("getPlant", [](BlockIndex& block) -> Plant& { return block.m_hasPlant.get(); }, nb::rv_policy::reference)
		.def_ro("getFluids", &BlockIndex::m_fluids, nb::rv_policy::reference)
		.def("addFluid", &BlockIndex::addFluid);
	// CauseOfDeath
	nb::enum_<CauseOfDeath>(m, "CauseOfDeath")
		.value("none", CauseOfDeath::none)
		.value("thirst", CauseOfDeath::thirst)
		.value("hunger", CauseOfDeath::hunger)
		.value("bloodLoss", CauseOfDeath::bloodLoss)
		.value("wound", CauseOfDeath::wound)
		.value("temperature", CauseOfDeath::temperature)
		.export_values();
	// Wound
	nb::class_<Wound>(m, "Wound")
		.def_ro("woundType", &Wound::woundType)
		.def_ro("bleedVolumeRate", &Wound::bleedVolumeRate)
		.def("getPercentHealed", &Wound::getPercentHealed)
		.def("impairPercent", &Wound::impairPercent);
	// BodyPart
	nb::class_<BodyPart>(m, "BodyPart")
		.def("bodyPartType", [](const BodyPart& bodyPart){ return bodyPart.bodyPartType; }, nb::rv_policy::reference)
		.def("materialType", [](const BodyPart& bodyPart){ return bodyPart.materialType; }, nb::rv_policy::reference)
		.def_ro("wounds", &BodyPart::wounds)
		.def_ro("severed", &BodyPart::severed);
	// Body
	nb::class_<Body>(m, "Body")
		.def_ro("bodyParts", &Body::m_bodyParts, nb::rv_policy::reference)
		.def("getVolume", &Body::getVolume)
		.def("isBleeding", &Body::hasBleedEvent)
		.def("getImpairMovePercent", &Body::getImpairMovePercent)
		.def("getImpairManipulationPercent", &Body::getImpairManipulationPercent);
	// Attributes
	nb::class_<Attributes>(m, "Attributes")
		.def("getStrength", &Attributes::getStrength)
		.def("getAgility", &Attributes::getAgility)
		.def("getDextarity", &Attributes::getDextarity);
	// EquipmentSet
	nb::class_<EquipmentSet>(m, "EquipmentSet")
		.def("list", &EquipmentSet::getAll);
	// Actor
	nb::class_<Actor>(m, "Actor")
		.def_ro("id", &Actor::m_id)
		.def_rw("name", &Actor::m_name)
		.def("species", [](const Actor& actor){ return actor.m_species; }, nb::rv_policy::reference)
		.def_ro("alive", &Actor::isAlive())
		.def_ro("causeOfDeath", &Actor::m_causeOfDeath)
		.def_ro("body", &Actor::m_body, nb::rv_policy::reference)
		.def_ro("attributes", &Actor::m_attributes)
		.def_ro("skillSet", &Actor::m_skillSet)
		.def_ro("equipmentSet", &Actor::m_equipmentSet)
		.def("starvationPercent", [](const Actor& actor){ return actor.m_mustEat.getPercentStarved(); } )
		.def("dieOfThirstPercent", [](const Actor& actor){ return actor.m_mustDrink.getPercentDeadFromThirst(); } )
		.def("tiredPercent", [](const Actor& actor){ return actor.m_mustSleep.getTiredPercent(); } )
		.def("sleepPercent", [](const Actor& actor){ return actor.m_mustSleep.getSleepPercent(); } )
		.def("needsSleep", [](const Actor& actor){ return actor.m_mustSleep.getNeedsSleep(); } )
		.def("dieOfTemperaturePercent", [](const Actor& actor){ return actor.m_needsSafeTemperature.dieFromTemperaturePercent(); } )
		.def("getCarrying", [](Actor& actor){ return actor.m_canPickup.getCarrying(); }, nb::rv_policy::reference)
		.def("isCarrying", [](const Actor& actor){ return actor.m_canPickup.exists(); })
		.def("canMove", [](const Actor& actor){ return actor.m_canMove.canMove(); })
		.def("setDestination",[](Actor& actor, BlockIndex& destination){ actor.m_canMove.setDestination(destination); })
		.def("setDestinationAdjacentTo",[](Actor& actor, BlockIndex& destination){ actor.m_canMove.setDestinationAdjacentTo(destination); })
		.def("setDestinationAdjacentToHasShape",[](Actor& actor, HasShape& hs){ actor.m_canMove.setDestinationAdjacentTo(hs); })
		.def("setTarget", [](Actor& actor, Actor& target){ actor.m_canFight.setTarget(target); })
		.def("isGrowing", [](const Actor& actor){ return actor.m_canGrow.isGrowing(); })
		.def("setPriorityForObjectiveType", [](Actor& actor, ObjectiveType& objectiveType, uint8_t priority){ actor.m_hasObjectives.m_prioritySet.setPriority(objectiveType,priority); })
		.def("removeObjectiveType", [](Actor& actor, ObjectiveType& objectiveType){ actor.m_hasObjectives.m_prioritySet.remove(objectiveType); })
		.def("currentObjectiveName", [](Actor& actor){ return actor.m_hasObjectives.getCurrent().name(); })
		.def("currentStamina", [](Actor& actor){ return actor.m_stamina.get(); })
		.def("maxStamina", [](Actor& actor){ return actor.m_stamina.getMax(); })
		.def_ro("canSee", &Actor::m_canSee)
		.def_ro("visionRange", &Actor::m_visionRange);
	// Item
	nb::class_<Item>(m, "Item")
		.def_ro("id", &Item::m_id)
		.def_rw("name", &Item::m_name)
		.def("itemType", [](Item& item){ return item.m_itemType; })
		.def("materialType", [](Item& item){ return item.m_materialType; })
		.def("quantity", &Item::getQuantity)
		.def_ro("quality", &Item::m_quality)
		.def("isWorkPiece", [](Item& item){ return item.m_craftJobForWorkPiece != nullptr; })
		.def_ro("installed", &Item::m_installed)
		.def("hasCargo", [](const Item& item){ return !item.m_hasCargo.empty(); })
		.def("getCargoItems", [](Item& item){ return item.m_hasCargo.getItems(); })
		.def("getCargoFluidType", [](const Item& item){ return item.m_hasCargo.getFluidType(); }, nb::rv_policy::reference)
		.def("getCargoFluidVolume", [](const Item& item){ return item.m_hasCargo.getFluidVolume(); });
}
