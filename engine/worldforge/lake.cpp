#include "../deserializationMemo.h"
#include "../config.h"
#include "lake.h"
#include "worldLocation.h"
#include <string>
Lake::Lake(const Json& data, DeserializationMemo& deserializationMemo) : name(data["name"].get<std::wstring>())
{
	for(const Json& locationData : data["locations"])
		locations.push_back(&deserializationMemo.getLocationByNormalizedLatLng(locationData));
}
Json Lake::toJson() const
{
	Json data{{"name", name}, {"locations", Json::array()}};
	for(WorldLocation* location : locations)
		data["locations"].push_back(location);
	return data;
}
