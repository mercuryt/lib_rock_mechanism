#pragma once
#include <cstdint>
#include <array>
#include <vector>
template<typename T, int turnsPerCycle>
class Buckets
{
private:
	std::array<std::vector<T*>, turnsPerCycle> buckets;
	int bucketFor(int x)
	{
		return x % turnsPerCycle;
	}
public:
	void add(T& x)
	{
		buckets[bucketFor(x.m_id)].push_back(&x);
	}
	void remove(T& x)
	{
		std::erase(buckets.at(bucketFor(x.m_id)), &x);
	}
	std::vector<T*>& get(int step)
	{
		return buckets[bucketFor(step)];
	}
};
