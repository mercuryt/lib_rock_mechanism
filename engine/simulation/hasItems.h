#pragma once
#include "../item.h"
#include "../types.h"

struct DeserializationMemo;

class SimulationHasItems final
{
	Simulation& m_simulation;
	std::unordered_map<ItemId, Item> m_items;
	ItemId m_nextId = 0;
public:
	SimulationHasItems(Simulation& simulation) : m_simulation(simulation) { }
	SimulationHasItems(const Json data, DeserializationMemo& deserializationMemo, Simulation& simulation);
	Item& createItem(ItemParamaters itemParamaters);
	// Non generic, no id
	Item& createItemNongeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, CraftJob* cj = nullptr);
	// Generic, no id.
	Item& createItemGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity, CraftJob* cj = nullptr);
	// Non generic with id.
	Item& loadItemNongeneric(const ItemId id, const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, std::wstring name, CraftJob* cj = nullptr);
	// Generic, with id.
	Item& loadItemGeneric(const ItemId id, const ItemType& itemType, const MaterialType& materialType, Quantity quantity, CraftJob* cj = nullptr);
	void destroyItem(Item& item);
	void clearAll();
	Item& loadItemFromJson(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] ItemId nextId() { return ++m_nextId; }
	[[nodiscard]] Item& getById(ItemId id);
	[[nodiscard]] Json toJson() const;
};
