 #include "climate.h"
 #include "world.h"
std::array<Percent, 4> climate::humidityBySeason(const World& world, Point3D point)
{
	Percent yearRoundHumidity = world.getHumidity(point);
	Distance equator = world.m_boundry.sizeY() / 2;
	bool northernHemisphere = point.y() >= equator;
	Distance distanceFromEquator{(DistanceWidth)std::abs(point.y().get() - equator.get())};
	float ratioDistanceFromEquator{(float)distanceFromEquator.get() / (float)equator.get()};
	std::array<Percent, 4> humidityBySeason;
	// Equatorial regions have high percepitation year round and two distinct wet seasons in spring and fall.
	if(ratioDistanceFromEquator < Config::World::tropicalBandStart)
	{
		return {
			yearRoundHumidity * Config::World::equitorialRainModifierLow,
			yearRoundHumidity * Config::World::equitorialRainModifierHigh,
			yearRoundHumidity * Config::World::equitorialRainModifierLow,
			yearRoundHumidity * Config::World::equitorialRainModifierHigh
		};
	}
	// Tropical regions have a single distinct wet season, depending on hemisphere.
	else if(ratioDistanceFromEquator < Config::World::subtropicalBandStart)
	{
		if(northernHemisphere)
			return {
				yearRoundHumidity * Config::World::tropicalRainModifierHigh,
				yearRoundHumidity * Config::World::tropicalRainModifierHigh,
				yearRoundHumidity * Config::World::tropicalRainModifierLow,
				yearRoundHumidity * Config::World::tropicalRainModifierLow
			};
		else
			return {
				yearRoundHumidity * Config::World::tropicalRainModifierLow,
				yearRoundHumidity * Config::World::tropicalRainModifierLow,
				yearRoundHumidity * Config::World::tropicalRainModifierHigh,
				yearRoundHumidity * Config::World::tropicalRainModifierHigh
			};
	}
	// SubTropical regions are dry year round, but coastal areas are wet in the winter.
	else if(ratioDistanceFromEquator < Config::World::midLatitudeBandStart)
	{
			return {
				yearRoundHumidity * Config::World::subtropicalRainModifierLow,
				yearRoundHumidity * Config::World::subtropicalRainModifierLow,
				yearRoundHumidity * Config::World::subtropicalRainModifierLow,
				yearRoundHumidity * Config::World::subtropicalRainModifierHigh
			};
	}
	else if(ratioDistanceFromEquator < Config::World::polarBandStart)
	// Midlatitude regions are moderate to high, with a focus on western coasts during winter and eastern / central regions durring summer.
	{
			return {
				yearRoundHumidity * Config::World::midlatitudeRainModifierLow,
				yearRoundHumidity * Config::World::midlatitudeRainModifierHigh,
				yearRoundHumidity * Config::World::midlatitudeRainModifierLow,
				yearRoundHumidity * Config::World::midlatitudeRainModifierLow
			};
	}
	// Polar regions have low percpitation, with a small bump in the summer.
	else
	{
			return {
				yearRoundHumidity * Config::World::polarRainModifierLow,
				yearRoundHumidity * Config::World::polarRainModifierHigh,
				yearRoundHumidity * Config::World::polarRainModifierLow,
				yearRoundHumidity * Config::World::polarRainModifierLow
			};
	}
}
std::array<Temperature, 2> climate::temperatureMinAndMax(const World& world, Point3D point)
{
	std::array<Temperature, 2> output;
	Temperature baseTemperature = world.getAverageTemperature(point);
	Temperature max = baseTemperature + Config::World::maxAmbiantSwing / 2;
	Temperature min = baseTemperature - Config::World::maxAmbiantSwing / 2;
	return {min, max};
}