#include "bodyType.h"
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
std::string& BodyPartType::getName(BodyPartTypeId id) { return bodyPartTypeData.m_name[id]; };
Volume BodyPartType::getVolume(BodyPartTypeId id) { return bodyPartTypeData.m_volume[id]; };
bool BodyPartType::getDoesLocamotion(BodyPartTypeId id) { return bodyPartTypeData.m_doesLocamotion[id]; };
bool BodyPartType::getDoesManipulation(BodyPartTypeId id) { return bodyPartTypeData.m_doesManipulation[id]; };
bool BodyPartType::getVital(BodyPartTypeId id) { return bodyPartTypeData.m_vital[id]; };
std::vector<std::pair<AttackTypeId, MaterialTypeId>>& BodyPartType::getAttackTypesAndMaterials(BodyPartTypeId id) { return bodyPartTypeData.m_attackTypesAndMaterials[id]; };

bool BodyType::hasBodyPart(BodyTypeId id, BodyPartTypeId bodyPartType)
{
	return std::ranges::find(bodyTypeData.m_bodyPartTypes[id], bodyPartType) != bodyTypeData.m_bodyPartTypes[id].end();
}
void BodyType::create(std::string name, std::vector<BodyPartTypeId>& bodyPartTypes)
{
	bodyTypeData.m_name.add(name);
	bodyTypeData.m_bodyPartTypes.add(bodyPartTypes);
}
BodyTypeId BodyType::byName(std::string name)
{
	auto found = bodyTypeData.m_name.find(name);
	assert(found != bodyTypeData.m_name.end());
	return BodyTypeId::create(found - bodyTypeData.m_name.begin());
}
std::string BodyType::getName(BodyTypeId id) { return bodyTypeData.m_name[id]; }
std::vector<BodyPartTypeId>& BodyType::getBodyPartTypes(BodyTypeId id) { return bodyTypeData.m_bodyPartTypes[id]; }
