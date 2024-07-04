#pragma once
#include <unordered_map>
#include "../lib/dynamic_bitset.hpp"

template<typename Key>
concept PositiveInteger = std::is_integral_v<Key> && std::is_unsigned_v<Key>;

template<typename Key, typename Value, uint32_t initalSize = 512>
class BloomHashMap
{
	static_assert(PositiveInteger<Key>, "Key must be a positive integer");
private:
	//TODO: use ska::bytell_hash_map
	std::unordered_map<Key, Value> map;
	sul::dynamic_bitset<> bitset;
public:
	BloomHashMap() : bitset(initalSize) {}
	auto insert(const Key& key, const Value& value)
	{
		assert(!contains(key)); 
		// If an explicit initalSize of 0 is passed assume the size is to be set once and never change.
		if constexpr(initalSize)
			if(bitset.size() <= key)
				bitset.resize(key * 1.5);
		bitset.set(key);
		return map.insert(std::make_pair(key, value));
	}
	auto try_emplace(const Key& key, const Value& value)
	{
		if(!contains(key))
			return std::make_pair(map.end(), false);
		else
			return map.try_emplace(key, value);
	}
	auto erase(const Key& key)
	{
		assert(contains(key)); 
		bitset.reset(key);
		return map.erase(key);
	}
	auto maybeErase(const Key& key)
	{
		bitset.reset(key);
		return map.erase(key);
	}
	void clear() 
	{
		map.clear();
		bitset.reset();
	}
	void resize(Key size) { bitset.resize(size); }
	void moveKey(Key oldKey, Key newKey)
	{
		bitset[newKey] = bitset[oldKey];
		if(bitset[newKey])
			std::swap(map[newKey], map[oldKey]);
		else
			map.erase(newKey);
	}
	Value& operator[](const Key& key) 
	{
		if(!contains(key))
			bitset.set(key);
	       	return map[key]; 
	}
	[[nodiscard]] typename std::unordered_map<Key, Value>::iterator find(const Key& key)
	{
		if(!contains(key))
			return map.end();
		return map.find(key);
	}
	[[nodiscard]] typename std::unordered_map<Key, Value>::const_iterator find(const Key& key) const
	{
		return const_cast<BloomHashMap<Key, Value>*>(this)->find(key);
	}
	[[nodiscard]] bool contains(const Key& key) const { return bitset.test(key); }
	[[nodiscard]] size_t size() const { return map.size(); }
	[[nodiscard]] bool empty() const { return map.empty(); }
	[[nodiscard]] const Value& at(const Key& key) const { return map.at(key); }
	[[nodiscard]] Value& at(const Key& key) { return map.at(key); }
	[[nodiscard]] const Value& operator[](const Key& key) const { return map.at(key); }
	[[nodiscard]] auto end() const { return map.end(); }
};
