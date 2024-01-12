#include "../../lib/doctest.h"
#include "../../engine/buckets.h"
struct Mock
{
	uint32_t m_id;
	Mock(uint32_t id) : m_id(id) { }
};
inline bool contains(std::vector<Mock*>& v, Mock& m)
{
	return std::find(v.begin(), v.end(), &m) != v.end();
}
TEST_CASE("bucket")
{
	Buckets<Mock, 2> buckets;
	Mock m1(1);
	Mock m2(2);
	buckets.add(m1);
	CHECK(buckets.get(1).size() == 1);
	CHECK(buckets.get(2).size() == 0);
	CHECK(contains(buckets.get(1), m1));
	buckets.add(m2);
	CHECK(buckets.get(2).size() == 1);
	CHECK(contains(buckets.get(2), m2));
	buckets.remove(m1);
	CHECK(buckets.get(1).size() == 0);
	CHECK(buckets.get(2).size() == 1);
	CHECK(contains(buckets.get(2), m2));
}
