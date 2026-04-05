#include "physics.h"
#include <fstream>
void Config::Physics::load()
{
	std::ifstream f("data/config/physics.json");
	Json data = Json::parse(f);
	data["volumeOfFluidToGenerateWhenAPointFeatureMelts"].get_to(volumeOfFluidToGenerateWhenAPointFeatureMelts);
	data["rangeOfInfluenceForPortalsBetweenOutsideAndInside"].get_to(rangeOfInfluenceForPortalsBetweenOutsideAndInside);
	data["maxVolumeOfInfluenceForPortalsBetweenOutsideAndInside"].get_to(maxVolumeOfInfluenceForPortalsBetweenOutsideAndInside);
	data["radiantHeatDisipatesAtDistanceExponent"].get_to(radiantHeatDisipatesAtDistanceExponent);
	data["minimumHeatDeltaToTrackAffectedArea"].get_to(minimumHeatDeltaToTrackAffectedArea);
	data["ambiantTemperatureDeltaDecay"].get_to(ambiantTemperatureDeltaDecay);
}