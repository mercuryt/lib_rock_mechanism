FluidContents::FluidContents(Block& block) : m_block(block), m_totalVolume(0) {}
bool FluidContents::isOverfull() const { return m_totalVolume > s_maxBlockVolume; }
bool FluidContents::contains(const FluidType* fluidType) const 
{
       	return std::ranges::find(m_data, fluidType, VolumeAndGroup::group::m_fluidType) != m_data.end(); 
}
bool FluidContents::contains(const FluidGroup& fluidGroup) const
{
       	return std::ranges::find(m_data, &fluidGroup, VolumeAndGroup::group) != m_data.end(); 
}
VolumeAndGroup* FluidContents::volumeAndGroupFor(const FluidType* fluidType) const
{
	auto found = std::ranges::find(m_data, fluidType, VolumeAndGroup::group::m_fluidType);
	if(found == m_data.end())
		return nullptr;
	return &*found;
}
VolumeAndGroup* FluidContents::volumeAndGroupFor(const FluidGroup& fluidGroup) const
{
	auto found = std::ranges::find(m_data, fluidGroup, VolumeAndGroup::group);
	if(found == m_data.end())
		return nullptr;
	return &*found;
}
bool FluidContents::canEnter(const FluidType* fluidType) const
{
	if(m_totalVolume < s_maxBlockVolume)
		return true;
	for(auto& data : m_data)
		if(data.group.m_fluidType.density < fluidType.density)
			return true;
	return false;
}
uint32_t FluidContents::volumeCanEnter(const FluidType* fluidType) const
{
	assert(canEnter(fluidType));
	if(m_data.empty())
		return s_maxBlockVolume;
	uint32_t output = s_maxBlockVolume - m_totalVolume;
	//TODO: sort here?
	for(VolumeAndGroup& volumeAndGroup : m_data)
		if(volumeAndGroup.group->m_fluidType->density < fluidType->density)
			output += volumeAndGroup.volume;
	return output;
}
void FluidContents::addVolume(const FluidType* fluidType, uint32_t volume)
{
	VolumeAndGroup* volumeAndGroup = volumeAndGroupFor(fluidType);
	assert(volumeAndGroup != nullptr);
	volumeAndGroup.volume += volume;
	m_totalVolume += volume;
}
void FluidContents::addVolume(const FluidGroup& fluidGroup, uint32_t volume)
{
	VolumeAndGroup* volumeAndGroup = volumeAndGroupFor(fluidGroup);
	if(volumeAndGroup == nullptr)
		m_data.emplace_back(volume, &fluidGroup);
	else
	{
		volumeAndGroup.volume += volume;
		m_totalVolume += volume;
	}
}
void FluidContents::removeVolume(const FluidType* fluidType, uint32_t volume)
{
	auto found = std::ranges::find(m_data, fluidType, VolumeAndGroup::group::m_fluidType);
	assert(found != m_data.end());
	volumeAndGroup->volume -= volume;
	m_totalVolume -= volume;
	if(volumeAndGroup->volume == 0)
		m_data.erase(found);
}
void FluidContents::clear()
{
	m_totalVolume = 0;
	m_data.clear();
}
void FluidContents::sortByDensityAscending()
{
	std::sort(m_data, std::less, VolumeAndGroup::group::m_fluidType::density);
}
void FluidContents::resolveOvefull()
{
	assert(m_block.m_fluidContents == this);
	assert(m_totalVolume > s_maxBlockVolume);
	sortByDensityAscending();
	for(VolumeAndGroup& volumeAndGroup : m_data)
	{
		uint32_t volumeToRemove = std::min(m_totalVolume - s_maxBlockVolume, volumeAndGroup.volume);
		volumeAndGroup.volume -= volumeToRemove;
		m_totalVolume -= volumeToRemove;
		if(volumeAndGroup.volume == 0)
			volumeAndGroup.group.removeBlock(m_block);
		if(m_totalVolume == s_maxBlockVolume)
			break;
	}
	std::erase_if(m_data, [&](VolumeAndGroup& volumeAndGroup){ return volumeAndGroup.volume == 0; });
}
