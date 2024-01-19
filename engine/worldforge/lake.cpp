#include "../deserializationMemo.h"
#include "../config.h"
#include "lake.h"
#include "worldLocation.h"
Lake::Lake(const Json& data, DeserializationMemo& deserializationMemo) : name(data["name"])
{
	for(const Json& locationData : data["locations"])
	{
		WorldLocation& location = deserializationMemo.getLocationByLatLng(locationData);
	}
}
Json Lake::toJson() const
{
	Json data{{"name", name}, {"locations", Json::array()}};
	for(WorldLocation* location : locations)
		data["locations"].push_back(location);
	return data;
}
