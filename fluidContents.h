// Needs to be copy constructable so it can be stored in a vector and also sorted by density for resolveOverfull.
class FluidGroup;
class Block;
struct VolumeAndGroup
{
	uint32_t volume;
	FluidGroup* group;
};
static_assert(std::copy_constructible<VolumeAndGroup>);
class FluidContents
{
	Block& m_block;
	std::vector<VolumeAndGroup> m_data;
	uint32_t m_totalVolume;

public:

	FluidContents();
	bool isOverfull() const;
	bool contains(const FluidType* fluidType) const;
	bool contains(const FluidGroup& fluidGroup) const;
	VolumeAndGroup* volumeAndGroupFor(const FluidType* fluidType) const;
	VolumeAndGroup* volumeAndGroupFor(const FluidGroup& fluidGroup) const;
	bool canEnter(const FluidType* fluidType) const;
	uint32_t volumeCanEnter(const FluidType* fluidType) const;
	void addVolume(const FluidType* fluidType, uint32_t volume);
	void addVolume(const FluidGroup& fluidGroup, uint32_t volume);
	void removeVolume(const FluidType* fluidType, uint32_t volume);
	void clear();
	void sortByDensityAscending();
	void resolveOvefull();
	std::vector<VolumeAndGroup>::iter begin() const { return m_data.begin(); }
	std::vector<VolumeAndGroup>::iter end() const { return m_data.end(); }
};
