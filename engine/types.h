// One Block has xyz dimensions of 1 meter by 1 meter by 2 meters.
//
#pragma once
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