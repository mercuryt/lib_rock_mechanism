#pragma once

#include <string>
#include <list>

struct BlockFeatureType
{
	const std::string name;

	BlockFeatureType(std::string n) : name(n) {}
};

static std::list<BlockFeatureType> blockFeatureTypes;

const BlockFeatureType* registerBlockFeatureType(std::string name)
{
	blockFeatureTypes.emplace_back(name);
	return &blockFeatureTypes.back();
}
