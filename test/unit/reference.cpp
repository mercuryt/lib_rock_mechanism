#include "../../lib/doctest.h"
#include "../../engine/reference.h"
#include "../../engine/types.h"

TEST_CASE("reference")
{
	ReferenceData<ItemIndex, ItemReferenceIndex> referenceData;
	ItemIndex index = ItemIndex::create(0);
	referenceData.add(index);
	ItemReference ref = referenceData.getReference(index);
	assert(ref.getIndex(referenceData) == index);
	assert(referenceData.getIndices().size() == 1);
	assert(referenceData.getReferenceIndices().size() == 1);
	assert(referenceData.getUnusedReferenceIndices().size() == 0);
	SUBCASE("remove")
	{
		referenceData.remove(index);
		assert(referenceData.getIndices().size() == 0);
		assert(referenceData.getReferenceIndices().size() == 0);
		assert(referenceData.getUnusedReferenceIndices().size() == 0);
	}
	SUBCASE("add")
	{
		ItemIndex index2 = ItemIndex::create(1);
		referenceData.add(index2);
		ItemReference ref2 = referenceData.getReference(index2);
		assert(referenceData.getIndices().size() == 2);
		assert(referenceData.getReferenceIndices().size() == 2);
		assert(referenceData.getUnusedReferenceIndices().size() == 0);
		ref.validate(referenceData);
		ref2.validate(referenceData);
		assert(referenceData.getIndices().contains(index));
		assert(referenceData.getIndices().contains(index2));
		SUBCASE("remove second")
		{
			referenceData.remove(index2);
			ref.validate(referenceData);
			assert(referenceData.getIndices().size() == 1);
			assert(referenceData.getReferenceIndices().size() == 1);
			assert(referenceData.getUnusedReferenceIndices().size() == 0);
			assert(referenceData.getIndices().contains(ItemIndex::create(0)));
		}
		SUBCASE("remove first")
		{
			referenceData.remove(index);
			ref2.validate(referenceData);
			assert(referenceData.getIndices().size() == 2);
			assert(referenceData.getReferenceIndices().size() == 1);
			assert(referenceData.getUnusedReferenceIndices().size() == 1);
			assert(referenceData.getIndices().contains(ItemIndex::create(0)));
		}
	}
}