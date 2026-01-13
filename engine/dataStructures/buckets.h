#pragma once
#include <cstdint>
#include <array>
#include <vector>
template<typename T, int32_t turnsPerCycle>
class Buckets
{
private:
	std::array<std::vector<T*>, turnsPerCycle> buckets;
	int32_t bucketFor(int x)
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
	std::vector<T*>& get(int32_t step)
	{
		return buckets[bucketFor(step)];
	}
};
