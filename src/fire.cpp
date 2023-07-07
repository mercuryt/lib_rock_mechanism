void FireEvent::execute()
{
	if(!m_fire.m_hasPeaked &&m_fire.m_stage == FireStage::Smouldering)
	{
		m_fire.m_stage = FireStage::Burining;
		m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature / Config::heatFractionForBurn);
		m_fire.m_event.schedule(m_fire.m_materialType.burnStageDuration, m_fire);
	}
	else if(!m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burining)
	{
		m_fire.m_stage = FireStage::Flaming;
		m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature);
		m_fire.m_event.schedule(m_fire.m_materialType.flameStageDuration, m_fire);
		//TODO: schedule event to turn construction / solid into wreckage.
	}
	else if(m_fire.m_stage == FireStage::Flaming)
	{
		m_fire.m_hasPeaked = true;
		m_fire.m_stage = FireStage::Burining;
		m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature / Config::heatFractionForBurn);
		uint32_t delay = m_fire.m_materialType.burnStageDuration / Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(delay, m_fire);
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burining)
	{
		m_fire.m_stage = FireStage::Smouldering;
		m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature / Config::heatFractionForSmoulder);
		uint32_t delay = m_fire.m_materialType.burnStageDuration / Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(delay, m_fire);
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Smouldering)
	{
		// Clear the event pointer so ~Fire doesn't try to cancel the event which is currently executing.
		m_fire.m_event = nullptr;
		// Destroy m_fire by relasing the unique pointer in m_location.
		// Implicitly removes the influence of m_fire.m_temperatureSource.
		m_fire.m_location->m_area->m_hasFires.extinguish(m_fire);
	}
}
~FireEvent::FireEvent()
{
	m_fire.m_event.clearPointer();
}
Fire::Fire(Block& l, const MaterialType& mt) : m_location(&l), m_materialType(mt), m_stage(FireStage::Smouldering), m_hasPeaked(false), m_temperatureSource(m_materialType.flameTemperature / Config::heatFractionForSmoulder, l)
{
	m_event.schedule(m_materialType.burnStageDuration, *this);
}
void HasFires::ignite(Block& block, const MaterialType& materialType)
{
	assert(!m_fires.contains(&block) || std::find(m_fires.at(&block), materialType, &Fire::MaterialType) == m_fires.end());
	m_fires[&block].emplace_back(block, materialType);
}
void HasFires::extinguish(Fire& fire)
{
	assert(std::find(m_fires.at(&block), fire) != m_fires.end());
	fire.m_temperatureSource.unapply();
	m_fires.at(&fire.m_location).remove(fire);
}
