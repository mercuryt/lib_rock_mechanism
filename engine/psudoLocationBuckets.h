#pragma once

template<typename T, typename getBucketIndex, uint bucketCount, uint initalCapacity, float growthFactor = GoldenRatio>
class PsudoLocationBuckets
{
	using This = PsudoLocationBuckets<T, getBucketIndex, bucketCount, initalCapacity, growthFactor>;
	std::vector<T> m_data;
	std::vector<std::vector<T>::iterator> m_bucketStarts;
public:
	PsudoLocationBuckets()
	{
		// Reserve memory for both data and buckets.
		m_data.resize(initalCapacity);
		m_bucketStarts.resize(bucketCount)
		// Initalize buckets spread evenly over data.
		// Data is initalized to empty implicitly.
		uint initalCapacitySlotsPerBucket = initalCapacity / bucketCount;
		std::vector<T>::iterator dataIter = m_data.begin();
		for(auto& iter : m_bucketStarts)
		{
			iter = dataIter;
			dataIter += initalCapacitySlotsPerBucket;
		}
	}
	void add(const T& value)
	{
		uint index = getBucketIndex(value);
		assert(index < bucketCount);
		std::vector<T>::iterator end = (index == m_bucketStarts.size() - 1) ? 
			m_data.end() :
			m_bucketStarts[index + 1];
		auto foundEmptySlot = std::ranges::find(m_bucketStarts[index], end, T::null());
		if(foundEmptySlot == end)
		{
			// No slot open, extend this bucket.
			uint distance = std::distance(m_bucketStarts[index], m_bucketStarts[index + 1]);
			uint slotsToInsert = (distance * growthFactor) - distance;
			// Vector insert preserves sort order.
			foundEmptySlot = m_data.insert(end, slotsToInsert, T::null());
			// Add to all subsequent bucketStarts after the bucket we have extended.
			for(auto iter = m_bucketStarts.begin() + index + 1; iter != m_bucketStars.end(); ++iter)
				*iter += slotsToInsert;
		}
		*foundEmptySlot = value;
	}
	void remove(conts T& value)
	{
		uint index = getBucketIndex(value);
		assert(index < bucketCount);
		std::vector<T>::iterator end = (index == m_bucketStarts.size() - 1) ? 
			m_data.end() :
			m_bucketStarts[index + 1];
		auto found = std::ranges::find(m_bucketStarts[index], end, value);
		assert(found != end);
		auto firstEmpty = std::ranges::find(found, end, T::null());
		if(found + 1 == firstEmpty)
			// The found slot is the last one in the bucket or the next slot after found is the first empty.
			found->clear();
		else
		{
			// Swap found with the item before first empty and then truncate.
			*found = *(firstEmpty - 1);
			(firstEmpty - 1)->clear();
		}
	}
	template<typename Action>
	void doAction(std::vector<std::pair<uint, uint>>& bucketIndices, Action&& action)
	{
		for(auto iter = bucketIndices.begin(); iter != bucketIndices.end(); ++iter)
		{
			// Prefetch the start of the next set of bucket indices.
			if(iter + 1 != bucketIndices.end())
				__builtin_prefetch((iter + 1)->first);
			std::vector<T>::iterator start = m_bucketStarts[iter->first];
			std::vector<T>::iterator end = pair.second == m_bucketStarts.size() ?
				m_data.end() :
				m_bucketStarts[iter->second];
			for(;start != end; ++start)
			{
				// When we get to the first empty skip to the next bucket.
				if(start->empty())
					break;
				action(*start);
			}
		}
	}
	struct const_iterator
	{
		const This& m_data;
		const std::vector<std::pair<uint, uint>>& m_bucketIndices;
		std::vector<std::pair<uint, uint>>::iterator m_bucketIndicesIterator;
		std::vector<T>::iterator m_begin;
		std::vector<T>::iterator m_end;
	public:
		const_iterator(const This& data, const std::vector<std::pair<uint, uint>>& bucketIndices) :
			m_data(data),
			m_bucketIndices(bucketIndices),
			m_bucketIndicesIterator(m_bucketIndices.begin()),
			m_begin(data.m_bucketStarts[bucketIndices.front().first]),
			m_end(data.m_bucketStarts[bucketIndices.front().second]) { }
		const_iterator(const This& data, const std::vector<std::pair<uint, uint>>& bucketIndices, bool end) :
			m_data(data),
			m_bucketIndices(bucketIndices),
			m_bucketIndicesIterator(bucketIndices.end()),
			m_begin(data.m_bucketStarts[bucketIndices.back().second]),
			m_end(data.m_bucketStarts[bucketIndices.back().second])
			{
				assert(end);
			}
		const T& operator*() { return *m_begin; }
		void operator++(int)
		{
			++m_begin;
			if(m_begin == m_end || m_begin->empty())
			{
				++m_bucketIndicesIterator;
				if(m_bucketIndicesIterator != m_bucketIndices.end())
				{
					// Prefetch the start of the next set of bucket indices.
					if(m_bucketIndicesIterator + 1 != m_bucketIndices.end())
						__builtin_prefetch((m_bucketIndicesIterator + 1)->first);
					// Set the next begin / end.
					m_begin = m_data.bucketStarts[m_bucketIndicesIterator->first];
					// Index pairs should not be consecutive.
					assert(m_begin != m_end);
					m_end = m_data.bucketStarts[m_bucketIndicesIterator->end];
				}
			}
		}
		bool operator==(const iterator& other)
		{
			assert(m_data == other.m_data);
			assert(m_bucketIndices == other.m_bucketIndices);
			return m_bucketIndicesIterator == other.m_bucketIndicesIterator && m_begin == other.m_begin;
		}
	};
	const_iterator begin(const std::vector<std::pair<uint, uint>>& bucketIndices) const { return const_iterator(*this, bucketIndices); }
	const_iterator end(const std::vector<std::pair<uint, uint>>& bucketIndices) const { return const_iterator(*this, bucketIndices, true); }
};