 #pragma once

class util
{
public:
	template <typename F>
		static std::unordered_set<BLOCK*> collectAdjacentsWithCondition(F&& condition, BLOCK* block)
		{
			std::unordered_set<BLOCK*> output;
			std::unordered_set<BLOCK*> closedList;
			std::stack<BLOCK*> openList;
			openList.push(block);
			output.insert(block);
			while(!openList.empty())
			{
				BLOCK*& block = openList.top();
				openList.pop();
				for(BLOCK* adjacent : block->m_adjacentsVector)
					if(condition(adjacent) && !output.contains(adjacent))
					{
						output.insert(adjacent);
						openList.push(adjacent);
					}
			}
			return output;
		}
	// This was suposed to replace the above function for detecting splits in fluidGroups but it was slower
	template <typename F>
		static std::vector<std::unordered_set<BLOCK*>> findGroups(F&& condition, std::unordered_set<BLOCK*>& blocks)
		{
			std::vector<std::unordered_set<BLOCK*>> output;
			output.reserve(blocks.size());
			auto huristic = [&](BLOCK* block){ 
				uint32_t shortestDistance = UINT32_MAX;
				for(BLOCK* b : blocks)
				{
					uint32_t distance = b->taxiDistance(*block);
					if(shortestDistance > distance)
						shortestDistance = distance;
				}
				return shortestDistance;
			};
			auto compare = [&](BLOCK* a, BLOCK* b){ return huristic(a) > huristic(b); };
			std::priority_queue<BLOCK*, std::vector<BLOCK*>, decltype(compare)> open(compare);
			std::unordered_set<BLOCK*> closed;
			while(!blocks.empty())
			{
				auto it = blocks.begin();
				BLOCK* first = *it;
				blocks.erase(it);
				open.push(first);
				closed.insert(first);
				while(!open.empty())
				{
					BLOCK* candidate = open.top();
					open.pop();
					auto found = blocks.find(candidate);
					if(found != blocks.end())
					{
						blocks.erase(found);
						// All input blocks are connected, return an empty vector.
						if(blocks.empty() && output.empty())
							return output;
					}
					for(BLOCK* b : candidate->m_adjacentsVector)
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
