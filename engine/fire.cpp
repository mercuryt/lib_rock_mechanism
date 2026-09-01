#include "fire.h"
#include "area/area.h"
#include "simulation/simulation.h"
#include "definitions/materialType.h"
#include "space/space.h"
#include "numericTypes/types.h"
#include "numericTypes/idTypes.h"
#include "config/physics.h"
void FireData::nextPhase(Area& area, Cuboid cuboid)
{
	TemperatureDelta baseTemperature = MaterialType::getFlameTemperature(m_materialType);
	if(!m_hasPeaked && m_stage == FireStage::Smouldering)
	{
		m_stage = FireStage::Burning;
		TemperatureDelta oldDelta = baseTemperature * Config::heatFractionForSmoulder;
		TemperatureDelta newDelta = baseTemperature * Config::heatFractionForBurn;
		area.m_hasTemperature.m_sources.updateTemperatureSourceDelta(area, cuboid, oldDelta, m_temperatureSource, newDelta);
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + MaterialType::getBurnStageDuration(m_materialType), *this, cuboid);
	}
	else if(!m_hasPeaked && m_stage == FireStage::Burning)
	{
		m_stage = FireStage::Flaming;
		TemperatureDelta oldDelta = baseTemperature * Config::heatFractionForBurn;
		TemperatureDelta newDelta = baseTemperature;
		area.m_hasTemperature.m_sources.updateTemperatureSourceDelta(area, cuboid, oldDelta, m_temperatureSource, newDelta);
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + MaterialType::getFlameStageDuration(m_materialType), *this, cuboid);
	}
	else if(m_stage == FireStage::Flaming)
	{
		m_hasPeaked = true;
		m_stage = FireStage::Burning;
		TemperatureDelta oldDelta = baseTemperature;
		TemperatureDelta newDelta = baseTemperature * Config::heatFractionForBurn;
		area.m_hasTemperature.m_sources.updateTemperatureSourceDelta(area, cuboid, oldDelta, m_temperatureSource, newDelta);
		Step delay = MaterialType::getBurnStageDuration(m_materialType) * Config::fireRampDownPhaseDurationFraction;
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + delay, *this, cuboid);
		Space& space = area.getSpace();
		CuboidSet solidWithMaterialType = space.solid_getCuboidsWithMaterialType(cuboid.toSet(), m_materialType);
		space.solid_setNotAll(solidWithMaterialType);
		space.item_addChunksAndPiles(solidWithMaterialType, Config::Physics::volumeOfRubbleToGenerateWhenSolidBurns, m_materialType);
		CuboidSet featuresWithMaterialType = space.pointFeature_getCuboidsWithMaterialType(cuboid.toSet(), m_materialType);
		space.pointFeature_removeAllWithMaterialType(featuresWithMaterialType, m_materialType);
		space.item_addChunksAndPiles(featuresWithMaterialType, Config::Physics::volumeOfRubbleToGenerateWhenFeatureBurns, m_materialType);
	}
	else if(m_hasPeaked && m_stage == FireStage::Burning)
	{
		m_stage = FireStage::Smouldering;
		TemperatureDelta oldDelta = baseTemperature * Config::heatFractionForBurn;
		TemperatureDelta newDelta = baseTemperature * Config::heatFractionForSmoulder;
		area.m_hasTemperature.m_sources.updateTemperatureSourceDelta(area, cuboid, oldDelta, m_temperatureSource, newDelta);
		Step delay = MaterialType::getBurnStageDuration(m_materialType) * Config::fireRampDownPhaseDurationFraction;
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + delay, *this, cuboid);
	}
	else if(m_hasPeaked && m_stage == FireStage::Smouldering)
	{
		area.m_hasTemperature.m_sources.removeTemperatureSource(area, cuboid, m_temperatureSource);
		clear();
	}
}
void FireData::clear()
{
	m_temperatureSource.clear();
	m_materialType.clear();
	m_stage = FireStage::Smouldering;
	m_hasPeaked = false;
}
bool FireData::empty() const
{
	return m_temperatureSource.empty();
}
FireDelta FireData::createDelta(Cuboid cuboid) const { return { cuboid, m_materialType}; }
TemperatureDelta FireData::getTemperatureDelta() const
{
	float modifier;
	switch(m_stage)
	{
		case FireStage::Smouldering:
			modifier = Config::heatFractionForSmoulder;
		break;
		case FireStage::Burning:
			modifier = Config::heatFractionForBurn;
		break;
		case FireStage::Flaming:
			modifier = 1;
		break;
	}
	return TemperatureDelta::create(modifier * (float)MaterialType::getFlameTemperature(m_materialType).get());
}
std::string FireData::toS() const
{
	return "{source: " + m_temperatureSource.toS() + ", material: " + m_materialType.toS() + ", stage: " + std::to_string((int)m_stage) + ", peaked: " + std::to_string(m_hasPeaked) + "}";
}
FireData FireData::create(Area& area, Cuboid cuboid, MaterialTypeId materialType, bool hasPeaked, FireStage stage)
{
	auto temperatureSource = area.m_hasTemperature.m_sources.addTemperatureSource(area, cuboid, MaterialType::getFlameTemperature(materialType) * Config::heatFractionForSmoulder);
	return {temperatureSource, materialType, stage, hasPeaked};
}
void AreaHasFires::doStep(const Step step, Area& area)
{
	auto iter = m_deltas.find(step);
	if(iter == m_deltas.end())
		return;
	auto deltasForStep = std::move(iter->second);
	m_deltas.erase(iter);
	// Deltas may have become invalidated due to the fire being extinguished, missing fires are ignored.
	for(const FireDelta& delta : deltasForStep)
	{
		m_fires.updateOrDestroyActionWithConditionAll(
			delta.location,
			[&area](Cuboid cuboid, FireData& fireData){
				fireData.nextPhase(area, cuboid);
			},
			[material = delta.materialType](FireData fireData) -> bool {
				return fireData.m_materialType == material;
			}
		);
	}
}
void AreaHasFires::scheduleNextPhase(Step step, FireData fire, Cuboid cuboid)
{
	m_deltas.getOrCreate(step).push_back(fire.createDelta(cuboid));
}
void AreaHasFires::ignite(Area& area, const CuboidSet& cuboidSet, MaterialTypeId materialType)
{
	CuboidSet toIgnite = cuboidSet;
	m_fires.queryRemoveWithCondition(toIgnite, [materialType](FireData fire){ return fire.m_materialType == materialType; });
	for(Cuboid cuboid : toIgnite)
	{
		// Temperature source is created and it's id assigned by FireData::Create.
		FireData fire = FireData::create(area, cuboid, materialType);
		scheduleNextPhase(area.m_simulation.m_step + MaterialType::getBurnStageDuration(materialType), fire, cuboid);
		m_fires.insert(cuboid, fire);
	}
}
void AreaHasFires::extinguish(Area& area, FireData fire, Cuboid cuboid)
{
	m_fires.removeWithCondition(cuboid, [&fire](FireData otherFire) { return fire.m_materialType == otherFire.m_materialType; });
	area.m_hasTemperature.m_sources.removeTemperatureSource(area, cuboid, fire.m_temperatureSource);
}