#pragma once

#include <string>
#include <list>

struct MaterialType
{
	const std::string name;
	const uint32_t mass;
	const bool transparent;

	MaterialType(std::string n, uint32_t m, bool t) : name(n), mass(m), transparent(t) {}
};

static std::list<MaterialType> materialTypes;

inline const MaterialType* registerMaterialType(std::string name, uint32_t mass, bool transparent)
{
	materialTypes.emplace_back(name, mass, transparent);
	return &materialTypes.back();
}
