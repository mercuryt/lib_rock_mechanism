#include "locii.h"
#include "gps.h"
#include "types.h"
#include "world.h"
#include "worldLocation.h"
#include <cmath>
#include <ios>
#include <numbers>
Locii::Locii(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : intensity(data["intensity"].get<float>()), decay(data["decay"].get<float>()), sustain(data["sustain"].get<Kilometers>()), exponent(data["exponent"].get<float>()), maxRange(data["maxRange"].get<Kilometers>())
{
		for(const Json& position : data["positions"])
			positions.push_back(position.get<LatLng>());
}
Json Locii::toJson() const
{
	Json data{
		{"intensity", intensity}, {"decay", decay}, {"sustain", sustain}, {exponent, "exponent"}, {"maxRange", maxRange}, {"positions", Json::array()}
	};
	for(const LatLng& position : positions)
		data["positions"].push_back(position);
	return data;
}
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
float Locii::intensityAtDistance(Kilometers distance) const
{
	if(sustain >= distance)
		return intensity;
	auto modifiedDistance = distance - sustain;
	float intensityModifier = modifiedDistance * decay;
	return intensityModifier > 1.f ? 0.f : intensity * (1.f-intensityModifier);
};
Kilometers Locii::distanceTo(Latitude latitude, Longitude longitude) const
{
	bool found = false;
	Kilometers output = 0;
	for(const LatLng& position : positions)
	{
		Kilometers distance = position.distanceTo(latitude, longitude);
		if(!found || output > distance)
		{
			found = true;
			output = distance;
		}
	}
	if(!found)
		return UINT32_MAX;
	return output;
}
