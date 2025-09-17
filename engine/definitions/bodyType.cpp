#include "definitions/bodyType.h"
void BodyPartType::create(BodyPartTypeParamaters& p)
{
	bodyPartTypeData.m_name.add(p.name);
	bodyPartTypeData.m_volume.add(p.volume);
	bodyPartTypeData.m_doesLocamotion.add(p.doesLocamotion);
	bodyPartTypeData.m_doesManipulation.add(p.doesManipulation);
	bodyPartTypeData.m_vital.add(p.vital);
	bodyPartTypeData.m_attackTypesAndMaterials.add(p.attackTypesAndMaterials);
}
BodyPartTypeId BodyPartType::byName(std::string name)
{
	auto found = bodyPartTypeData.m_name.find(name);
	assert(found != bodyPartTypeData.m_name.end());
	return BodyPartTypeId::create(found - bodyPartTypeData.m_name.begin());
}
std::string& BodyPartType::getName(const BodyPartTypeId& id) { return bodyPartTypeData.m_name[id]; };
FullDisplacement BodyPartType::getVolume(const BodyPartTypeId& id) { return bodyPartTypeData.m_volume[id]; };
bool BodyPartType::getDoesLocamotion(const BodyPartTypeId& id) { return bodyPartTypeData.m_doesLocamotion[id]; };
bool BodyPartType::getDoesManipulation(const BodyPartTypeId& id) { return bodyPartTypeData.m_doesManipulation[id]; };
bool BodyPartType::getVital(const BodyPartTypeId& id) { return bodyPartTypeData.m_vital[id]; };
std::vector<std::pair<AttackTypeId, MaterialTypeId>>& BodyPartType::getAttackTypesAndMaterials(const BodyPartTypeId& id) { return bodyPartTypeData.m_attackTypesAndMaterials[id]; };

bool BodyType::hasBodyPart(const BodyTypeId& id, const BodyPartTypeId& bodyPartType)
{
	return std::ranges::find(g_bodyTypeData.m_bodyPartTypes[id], bodyPartType) != g_bodyTypeData.m_bodyPartTypes[id].end();
}
void BodyType::create(std::string name, std::vector<BodyPartTypeId>& bodyPartTypes)
{
	g_bodyTypeData.m_name.add(name);
	g_bodyTypeData.m_bodyPartTypes.add(bodyPartTypes);
}
BodyTypeId BodyType::byName(std::string name)
{
	auto found = g_bodyTypeData.m_name.find(name);
	assert(found != g_bodyTypeData.m_name.end());
	return BodyTypeId::create(found - g_bodyTypeData.m_name.begin());
}
std::string BodyType::getName(const BodyTypeId& id) { return g_bodyTypeData.m_name[id]; }
std::vector<BodyPartTypeId>& BodyType::getBodyPartTypes(const BodyTypeId& id) { return g_bodyTypeData.m_bodyPartTypes[id]; }
