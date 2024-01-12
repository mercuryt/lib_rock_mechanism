#include "../../lib/doctest.h"
#include "../../engine/definitions.h"
TEST_CASE("load")
{
	definitions::loadShapes();
	definitions::loadFluidTypes();
	definitions::loadMoveTypes();
	definitions::loadMaterialTypes();
	definitions::loadSkillTypes();
	definitions::loadItemTypes();
	definitions::loadPlantSpecies();
	definitions::loadBodyPartTypes();
	definitions::loadBodyTypes();
	definitions::loadAnimalSpecies();
}
