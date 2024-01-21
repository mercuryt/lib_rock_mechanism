#pragma once
#include "../util.h"
#include "../lib/json.hpp"
struct LatLng
{
	Latitude latitude;
	Longitude longitude;
	double distanceTo(Latitude latitude, Longitude longitude) const 
	{
		double dist;
		dist = sin(toRad(latitude)) * sin(toRad(latitude)) + cos(toRad(latitude)) * cos(toRad(latitude)) * cos(toRad(longitude - longitude));
		dist = acos(dist);
		dist = 6371 * dist;
		return dist;
	}
	static double toRad(double degree) { return degree/180 * M_PI; }
	Bearing getBearing(double lat2, double lng2) 
	{
		double dLon = (lng2-longitude);
		double y = sin(dLon) * cos(lat2);
		double x = cos(latitude)*sin(lat2) - sin(latitude)*cos(lat2)*cos(dLon);
		double brng = util::radiansToDegrees((atan2(y, x)));
		return (360 - std::fmod((brng + 360) , 360));
	}
	double distanceTo(LatLng& other) const { return distanceTo(other.latitude, other.longitude); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(LatLng, latitude, longitude);
};
struct Gps final : public LatLng
{
	int32_t metersAboveSeaLevel;
};
