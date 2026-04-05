#pragma once
#include "../dataStructures/smallSet.h"
#include "../numericTypes/index.h"
#include "../reference.h"


// To be used for updating changing temperature per step as well as for long term storage in PointsExposedToSky.
struct CollectionOfThingsAffectedByTemperature
{
	SmallSet<ActorReference> actors;
	SmallSet<ItemReference> items;
	CuboidSet plants;
	CuboidSet solidAndFeatures;
	CuboidSet fluid;
	void add(const ActorReference ref);
	void add(const ItemReference ref);
	void add(const PlantIndex plant);
	void addSolid(const CuboidSet space);
	void addFluid(const CuboidSet space);
	void remove(const ActorReference ref);
	void remove(const ItemReference ref);
	void remove(const PlantIndex plant);
	void removeSolid(const CuboidSet space);
	void removeFluid(const CuboidSet space);
	void prepare();
	void clear();
	void merge(CollectionOfThingsAffectedByTemperature& other);
}