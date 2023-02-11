template<typename T, uint32_t turnsPerCycle>
class Buckets
{
private:
	std::array<std::vector<T*>, turnsPerCycle> stacks;
	uint32_t bucketFor(uint32_t x)
	{
		return x % turnsPerCycle;
	}
public:
	void add(T* x)
	{
		stacks[bucketFor(x->m_id)].push_back(x);
	}
	void remove(T* x)
	{
		std::erase(stacks[bucketFor(x->m_id)], x);
	}
	std::vector<T*>& get(uint32_t currentTurn)
	{
		return stacks[bucketFor(currentTurn)];
	}
};
