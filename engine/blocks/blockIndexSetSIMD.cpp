#include "blockIndexSetSIMD.h"
#include "../index.h"

BlockIndexSetSIMD::BlockIndexSetSIMD(const Eigen::Array<int, 1, Eigen::Dynamic>& data)
{
	m_data = data.template cast<BlockIndexWidth>();
}
bool BlockIndexSetSIMD::contains(const BlockIndex& block) const { assert(block.exists()); return (m_data == block.get()).any(); }
BlockIndex BlockIndexSetSIMD::operator[](const uint& index) const { assert(index < m_data.size()); return BlockIndex::create(m_data[index]); }
uint BlockIndexSetSIMD::size() const
{
	Eigen::Array<bool, 1, Eigen::Dynamic> notNull = m_data != BlockIndex::null().get();
	return notNull.count();
}
uint BlockIndexSetSIMD::sizeWithNull() const { return m_data.size(); }
std::string BlockIndexSetSIMD::toString() const { return util::eigenToString(m_data); }
BlockIndexSetSIMD::WithNullView BlockIndexSetSIMD::includeNull() const { return WithNullView(*this); }
SmallSet<BlockIndex> BlockIndexSetSIMD::toSmallSet() const
{
	SmallSet<BlockIndex> output;
	output.reserve(m_data.size());
	for(const BlockIndexWidth& value : m_data)
		if(value != BlockIndex::null().get())
			output.insert(BlockIndex::create(value));
	return output;
}