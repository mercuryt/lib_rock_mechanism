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
	DataVector<std::string, BodyPartTypeId> m_name;
	DataVector<Volume, BodyPartTypeId> m_volume;
	DataBitSet<BodyPartTypeId> m_doesLocamotion;
	DataBitSet<BodyPartTypeId> m_doesManipulation;
	DataBitSet<BodyPartTypeId> m_vital;
	DataVector<std::vector<std::pair<AttackTypeId, MaterialTypeId>>, BodyPartTypeId> m_attackTypesAndMaterials;
public:
	static void create(BodyPartTypeParamaters& p);
	[[nodiscard]] static BodyPartTypeId byName(std::string name);
	[[nodiscard]] static std::string& getName(const BodyPartTypeId& id);
	[[nodiscard]] static Volume getVolume(const BodyPartTypeId& id);
	[[nodiscard]] static bool getDoesLocamotion(const BodyPartTypeId& id);
	[[nodiscard]] static bool getDoesManipulation(const BodyPartTypeId& id);
	[[nodiscard]] static bool getVital(const BodyPartTypeId& id);
	[[nodiscard]] static std::vector<std::pair<AttackTypeId, MaterialTypeId>>& getAttackTypesAndMaterials(const BodyPartTypeId& id);
};
inline BodyPartType bodyPartTypeData;
// For example biped, quadraped, bird, etc.
class BodyType final
{
	DataVector<std::string, BodyTypeId> m_name;
	DataVector<std::vector<BodyPartTypeId>, BodyTypeId> m_bodyPartTypes;
public:
	static bool hasBodyPart(const BodyTypeId& id, const BodyPartTypeId& bodyPartType);
	static void create(std::string name, std::vector<BodyPartTypeId>& bodyPartTypes);
	[[nodiscard]] static BodyTypeId byName(std::string name);
	[[nodiscard]] static std::string getName(const BodyTypeId& id);
	[[nodiscard]] static std::vector<BodyPartTypeId>& getBodyPartTypes(const BodyTypeId& id);
};
inline BodyType bodyTypeData;
