#include "types.h"
#include "blocks/blocks.h"
class Point3DIterator {

private:
	Blocks& m_blocks;
	Point3D m_current;
	Point3D m_end;

public:
	Point3DIterator(Blocks& blocks, const Point3D& start, const Point3D& _end) : m_blocks(blocks), m_current(start), m_end(_end) { }
	Point3DIterator& operator++() {
		if (m_current.x < m_end.x) {
			m_current.x++;
		} else if (m_current.y < m_end.y) {
			m_current.x = m_end.x;
			m_current.y++;
		} else if (m_current.z < m_end.z) {
			m_current.x = m_end.x;
			m_current.y = m_end.y;
			m_current.z++;
		}
		return *this;
	}
	Point3DIterator operator++(int) {
		Point3DIterator tmp = *this;
		++(*this);
		return tmp;
	}
	bool operator==(const Point3DIterator& other) const { return m_current == other.m_current; }
	bool operator!=(const Point3DIterator& other) const { return !(*this == other); }
	BlockIndex operator*() { return m_blocks.getIndex(m_current); }
	Point3DIterator begin() { return Point3DIterator(m_blocks, m_current, m_end); }
	Point3DIterator end() { return Point3DIterator(m_blocks, m_end, m_end); }
};
