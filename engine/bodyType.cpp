#include "bodyType.h"
void BodyPartType::create(BodyPartTypeParamaters& p)
{
	data.m_name.add(p.name);
	data.m_volume.add(p.volume);
	data.m_doesLocamotion.add(p.doesLocamotion);
	data.m_doesManipulation.add(p.doesManipulation);
	data.m_vital.add(p.vital);
	data.m_attackTypesAndMaterials.add(p.attackTypesAndMaterials);
}
