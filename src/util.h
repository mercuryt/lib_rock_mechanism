 #pragma once
class util
{
public:
	template <typename F>
	static std::unordered_set<Block*> collectAdjacentsWithCondition(F&& condition, Block& block)
	{
		std::unordered_set<Block*> output;
		std::stack<Block*> openList;
		openList.push(&block);
		output.insert(&block);
		while(!openList.empty())
		{
			Block* block = openList.top();
			openList.pop();
			for(Block* adjacent : block->m_adjacentsVector)
				if(condition(adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	static std::unordered_set<Block*> collectAdjacentsInRange(uint32_t range, Block& block)
	{
		auto condition = [&](Block* b){ return b->taxiDistance(block) <= range; };
		return collectAdjacentsWithCondition(condition, block);
	}
	static std::vector<Block*> collectAdjacentsInRangeVector(uint32_t range, Block& block)
	{
		auto condition = [&](Block* b){ return b->taxiDistance(block) <= range; };
		auto result = collectAdjacentsWithCondition(condition, block);
		std::vector<Block*> output(result.begin(), result.end());
		return output;
	}
	template<typename PathT, typename DestinationT>
	static Block* findWithPathCondition(PathT&& pathCondition, DestinationT&& destinationCondition, Block& block)
	{
		std::unordered_set<Block*> closedList;
		std::stack<Block*> openList;
		openList.push(block);
		while(!openList.empty())
		{
			Block* block = openList.top();
			openList.pop();
			closedList.push(block);
			for(Block* adjacent : block->m_adjacentsVector)
			{
				if(destinationCondition(adjacent))
					return adjacent;
				if(pathCondition(adjacent) && !closedList.contains(adjacent))
				{
					closedList.insert(adjacent);
					openList.push(adjacent);
				}
			}
		}
		return nullptr;
	}
	// This was suposed to replace collectAdjacentsWithCondition for detecting splits in fluidGroups but it was slower
	template <typename F>
	static std::vector<std::unordered_set<Block*>> findGroups(F&& condition, std::unordered_set<Block*>& blocks)
	{
		std::vector<std::unordered_set<Block*>> output;
		output.reserve(blocks.size());
		auto huristic = [&](Block* block){ 
			uint32_t shortestDistance = UINT32_MAX;
			for(Block* b : blocks)
			{
				uint32_t distance = b->taxiDistance(*block);
				if(shortestDistance > distance)
					shortestDistance = distance;
			}
			return shortestDistance;
		};
		auto compare = [&](Block* a, Block* b){ return huristic(a) > huristic(b); };
		std::priority_queue<Block*, std::vector<Block*>, decltype(compare)> open(compare);
		std::unordered_set<Block*> closed;
		while(!blocks.empty())
		{
			auto it = blocks.begin();
			Block* first = *it;
			blocks.erase(it);
			open.push(first);
			closed.insert(first);
			while(!open.empty())
			{
				Block* candidate = open.top();
				open.pop();
				auto found = blocks.find(candidate);
				if(found != blocks.end())
				{
					blocks.erase(found);
					// All input blocks are connected, return an empty vector.
					if(blocks.empty() && output.empty())
						return output;
				}
				for(Block* b : candidate->m_adjacentsVector)
					if(condition(b) && !closed.contains(b))
					{
						open.push(b);
						closed.insert(b);
					}
			}
			// Disconnected group found, store it
			output.resize(output.size() + 1);
			output.back().swap(closed);
		}
		return output;
	}
	int scaleByPercent(int base, uint32_t percent)
	{
		return (base * percent) / 100u;
	}
	int scaleByInversePercent(int base, uint32_t percent)
	{
		return scaleByPercent(base, 100 - percent);
	}
	int scaleByPercentRange(int min, int max, uint32_t percent)
	{
		return min + (((max - min) * percent) / 100u);
	}
	int scaleByFraction(int base, uint32_t numerator, uint32_t denominator)
	{
		return (base * numerator) / denominator;
	}
	int scaleByInverseFraction(int base, uint32_t numerator, uint32_t denominator)
	{
		return scaleByFraction(base, denominator - numerator, denominator);
	}

};
