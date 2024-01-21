#pragma once
#include "biome.h"
class DeserializationMemo;
class World;
struct WorldLocation;
#include <string>
#include <vector>
#include "../config.h"
struct Lake
{
	std::wstring name;
	std::vector<WorldLocation*> locations;
	uint8_t size;
	Lake(std::wstring n, uint8_t s) : name(n), size(s) { }
	Lake(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
};
