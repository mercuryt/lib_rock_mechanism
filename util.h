 #pragma once

class util
{
public:
	template <typename F>
	static std::unordered_set<Block*> collectAdjacentsWithCondition(F&& condition, Block* block)
	{
		std::unordered_set<Block*> output;
		std::unordered_set<Block*> closedList;
		std::stack<Block*> openList;
		openList.push(block);
		while(!openList.empty())
		{
			Block*& block = openList.top();
			openList.pop();
			for(Block* adjacent : block->m_adjacents)
				if(adjacent != nullptr && condition(adjacent) && !output.contains(adjacent))
				{
					output.insert(adjacent);
					openList.push(adjacent);
				}
		}
		return output;
	}
};
