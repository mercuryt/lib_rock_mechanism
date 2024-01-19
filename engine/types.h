// One Block has xyz dimensions of 1 meter by 1 meter by 2 meters.
//
#pragma once
#include <chrono>
#include <cmath>
#include <cstdint>
using Step = uint64_t;
using Temperature = uint32_t; // degrees kelvin.
using CollisionVolume = uint32_t; // 20 liters, onehundredth of a block.
using Volume = uint32_t; // 10 cubic centimeters.
using Mass = uint32_t; // grams.
using Density = uint32_t; // grams per 10 cubic centimeters.
using Force = uint32_t;
using Percent = int32_t;
using Facing = uint8_t;
using AreaId = uint32_t;
using ItemId = uint32_t;
using ActorId = uint32_t;
using HaulSubprojectId = uint32_t;
using ProjectId = uint32_t;
using Kilometers = uint32_t;
using Meters = int32_t;
using Latitude = double;
using Longitude = double;
using Bearing = double;
struct Gps;
#include "util.h"
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
	static double toRad(double degree)
	{
		return degree/180 * M_PI;
	}
	Bearing getBearing(double lat2, double lng2) 
	{
		double dLon = (lng2-longitude);
		double y = sin(dLon) * cos(lat2);
		double x = cos(latitude)*sin(lat2) - sin(latitude)*cos(lat2)*cos(dLon);
		double brng = util::radiansToDegrees((atan2(y, x)));
		return (360 - std::fmod((brng + 360) , 360));
	}
	double distanceTo(LatLng& other) const { return distanceTo(other.latitude, other.longitude); }
};
struct Gps final : public LatLng
{
	int32_t metersAboveSeaLevel;
};
