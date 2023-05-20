 #pragma once
template<class DerivedBlock>
class util
{
public:
	template <typename F>
	static std::unordered_set<DerivedBlock*> collectAdjacentsWithCondition(F&& condition, DerivedBlock& block)
	{
		std::unordered_set<DerivedBlock*> output;
		std::stack<DerivedBlock*> openList;
		openList.push(&block);
		output.insert(&block);
		while(!openList.empty())
		{
			DerivedBlock* block = openList.top();
			openList.pop();
			for(DerivedBlock* adjacent : block->m_adjacentsVector)
				if(condition(adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
	static std::unordered_set<DerivedBlock*> collectAdjacentsInRange(uint32_t range, DerivedBlock& block)
	{
		auto condition = [&](DerivedBlock* b){ return b->taxiDistance(block) <= range; };
		return collectAdjacentsWithCondition(condition, block);
	}
	static std::vector<DerivedBlock*> collectAdjacentsInRangeVector(uint32_t range, DerivedBlock& block)
	{
		auto condition = [&](DerivedBlock* b){ return b->taxiDistance(block) <= range; };
		auto result = collectAdjacentsWithCondition(condition, block);
		std::vector<DerivedBlock*> output(result.begin(), result.end());
		return output;
	}
	template<typename PathT, typename DestinationT>
	static DerivedBlock* findWithPathCondition(PathT&& pathCondition, DestinationT&& destinationCondition, DerivedBlock& block)
	{
		std::unordered_set<DerivedBlock*> closedList;
		std::stack<DerivedBlock*> openList;
		openList.push(block);
		while(!openList.empty())
		{
			DerivedBlock* block = openList.top();
			openList.pop();
			closedList.push(block);
			for(DerivedBlock* adjacent : block->m_adjacentsVector)
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
	static std::vector<std::unordered_set<DerivedBlock*>> findGroups(F&& condition, std::unordered_set<DerivedBlock*>& blocks)
	{
		std::vector<std::unordered_set<DerivedBlock*>> output;
		output.reserve(blocks.size());
		auto huristic = [&](DerivedBlock* block){ 
			uint32_t shortestDistance = UINT32_MAX;
			for(DerivedBlock* b : blocks)
			{
				uint32_t distance = b->taxiDistance(*block);
				if(shortestDistance > distance)
					shortestDistance = distance;
			}
			return shortestDistance;
		};
		auto compare = [&](DerivedBlock* a, DerivedBlock* b){ return huristic(a) > huristic(b); };
		std::priority_queue<DerivedBlock*, std::vector<DerivedBlock*>, decltype(compare)> open(compare);
		std::unordered_set<DerivedBlock*> closed;
		while(!blocks.empty())
		{
			auto it = blocks.begin();
			DerivedBlock* first = *it;
			blocks.erase(it);
			open.push(first);
			closed.insert(first);
			while(!open.empty())
			{
				DerivedBlock* candidate = open.top();
				open.pop();
				auto found = blocks.find(candidate);
				if(found != blocks.end())
				{
					blocks.erase(found);
					// All input blocks are connected, return an empty vector.
					if(blocks.empty() && output.empty())
						return output;
				}
				for(DerivedBlock* b : candidate->m_adjacentsVector)
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
};
