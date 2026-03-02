#pragma once
#include "../../engine/numericTypes/idTypes.h"
#include "../../engine/numericTypes/types.h"
#include "../../engine/config/config.h"
class Window;
enum class ContextMenuId
{
	Root,
	Dig,
	Construct,
	Null
};
namespace contextMenu
{
	void draw(Window& window);
	void root(Window& window);
	namespace controlls
	{
		void dig(Window& window);
		void construct(Window& window);
	}
	namespace submenus
	{
		void dig(Window& window);
		void construct(Window& window);
	}
}
struct ContextMenuState
{
	ContextMenuId current = ContextMenuId::Null;
	Point3D clickedOnPoint;
	MaterialTypeId materialType;
	PointFeatureTypeId featureType;
	FluidTypeId fluidType;
	int fluidVolume = Config::maxPointVolume.get();
	bool constructed = true;
	Percent percentGrown{100};
	PlantSpeciesId plantSpecies;
	ItemTypeId itemType;
	Percent wear{0};
	Quality quality{0};
	Quantity quantity{1};
	Facing4 facing = Facing4::North;
	bool installed = false;
	std::string name;
};