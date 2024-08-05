#pragma once
#include "types.h"
#include "dataVector.h"
#include "attackType.h"
#include <string>
struct BodyPartTypeParamaters final
{
	std::string name;
	Volume volume;
	bool doesLocamotion;
	bool doesManipulation;
	bool vital;
	std::vector<std::pair<AttackTypeId, MaterialTypeId>> attackTypesAndMaterials;
};
// For example: 'left arm', 'head', etc.
class BodyPartType final
{
	static BodyPartType data;
	DataVector<std::string, BodyPartTypeId> m_name;
	DataVector<Volume, BodyPartTypeId> m_volume;
	DataVector<bool, BodyPartTypeId> m_doesLocamotion;
	DataVector<bool, BodyPartTypeId> m_doesManipulation;
	DataVector<bool, BodyPartTypeId> m_vital;
	DataVector<std::vector<std::pair<AttackTypeId, MaterialTypeId>>, BodyPartTypeId> m_attackTypesAndMaterials;
public:
	static void create(BodyPartTypeParamaters& p);
	static BodyPartTypeId byName(std::string name);
	[[nodiscard]] static std::string& getName(BodyPartTypeId id) { return data.m_name.at(id); };
	[[nodiscard]] static Volume getVolume(BodyPartTypeId id) { return data.m_volume.at(id); };
	[[nodiscard]] static bool getDoesLocamotion(BodyPartTypeId id) { return data.m_doesLocamotion.at(id); };
	[[nodiscard]] static bool getDoesManipulation(BodyPartTypeId id) { return data.m_doesManipulation.at(id); };
	[[nodiscard]] static bool getVital(BodyPartTypeId id) { return data.m_vital.at(id); };
	[[nodiscard]] static std::vector<std::pair<AttackTypeId, MaterialTypeId>>& getAttackTypesAndMaterials(BodyPartTypeId id) { return data.m_attackTypesAndMaterials.at(id); };
};
// For example biped, quadraped, bird, etc.
class BodyType final
{
	static BodyType data;
	DataVector<std::string, BodyTypeId> m_name;
	DataVector<std::vector<BodyPartTypeId>, BodyTypeId> m_bodyPartTypes;
	static bool hasBodyPart(BodyTypeId id, BodyPartTypeId bodyPartType)
	{
		return std::ranges::find(data.m_bodyPartTypes.at(id), bodyPartType) != data.m_bodyPartTypes.at(id).end();
	}
	static BodyTypeId byName(std::string name);
	static void create(std::string name, std::vector<BodyPartTypeId> bodyPartTypes);
};
