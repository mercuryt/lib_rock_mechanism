#pragma once

#include "../types.h"

#include <vector>

class Actor;
class Block;
class Item;
struct MaterialType;
struct ItemType;
struct FluidType;

class BlockHasItems final
{
	Block& m_block;
	std::vector<Item*> m_items;
public:
	BlockHasItems(Block& b);
	// Non generic types have Shape.
	void add(Item& item);
	void remove(Item& item);
	Item& addGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void remove(const ItemType& itemType, const MaterialType& materialType, Quantity quantity);
	void setTemperature(Temperature temperature);
	void disperseAll();
	//Item* get(ItemType& itemType) const;
	[[nodiscard]] Quantity getCount(const ItemType& itemType, const MaterialType& materialType) const;
	[[nodiscard]] Item& getGeneric(const ItemType& itemType, const MaterialType& materialType) const;
	[[nodiscard]] std::vector<Item*>& getAll() { return m_items; }
	const std::vector<Item*>& getAll() const { return m_items; }
	[[nodiscard]] bool hasInstalledItemType(const ItemType& itemType) const;
	[[nodiscard]] bool hasEmptyContainerWhichCanHoldFluidsCarryableBy(const Actor& actor) const;
	[[nodiscard]] bool hasContainerContainingFluidTypeCarryableBy(const Actor& actor, const FluidType& fluidType) const;
	[[nodiscard]] bool empty() const { return m_items.empty(); }
};
