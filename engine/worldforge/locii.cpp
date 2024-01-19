#include "locii.h"
#include "world.h"
Locii Locii::make(LociiConfig& config, World& world)
{
	Random& random = world.m_random;
	float intensity = random.getInRange(config.minimumIntensity, config.maximumIntensity);
	Kilometers sustain = random.getInRange(config.minimumSustain, config.maximumSustain);
	float decay = random.getInRange(config.minimumDecay, config.maximumDecay);
	float exponent = random.getInRange(config.minimumExponent, config.maximumExponent);
	uint32_t positionsCount = random.getInRange(config.minimumPositions, config.maximumPositions);
	LatLng initialPosition = world.getRandomCoordinates();
	std::vector<LatLng> positions{initialPosition};
	for(uint i = 0; i < positionsCount; ++i)
		positions.push_back(world.getRandomCoordinatesInRangeOf(initialPosition, sustain));
	return {positions, intensity, decay, sustain, exponent};
}
