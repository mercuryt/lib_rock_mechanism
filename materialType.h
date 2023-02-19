#pragma once

#include <string>
#include <list>

struct MaterialType
{
	const std::string name;
	const uint32_t mass;

	MaterialType(std::string n, uint32_t m) : name(n), mass(m) {}
};

static std::list<MaterialType> materialTypes;

MaterialType* registerMaterialType(std::string name, uint32_t mass)
{
	materialTypes.emplace_back(name, mass);
	return &materialTypes.back();
}
