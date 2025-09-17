#pragma once
#include "numericTypes/types.h"
#include "dataStructures/strongVector.h"
#include "definitions/attackType.h"
#include <string>
struct BodyPartTypeParamaters final
{
	std::string name;
	FullDisplacement volume;
	bool doesLocamotion;
	bool doesManipulation;
	bool vital;
	std::vector<std::pair<AttackTypeId, MaterialTypeId>> attackTypesAndMaterials;
};
// For example: 'left arm', 'head', etc.
class BodyPartType final
{
	StrongVector<std::string, BodyPartTypeId> m_name;
	StrongVector<FullDisplacement, BodyPartTypeId> m_volume;
	StrongBitSet<BodyPartTypeId> m_doesLocamotion;
	StrongBitSet<BodyPartTypeId> m_doesManipulation;
	StrongBitSet<BodyPartTypeId> m_vital;
	StrongVector<std::vector<std::pair<AttackTypeId, MaterialTypeId>>, BodyPartTypeId> m_attackTypesAndMaterials;
public:
	static void create(BodyPartTypeParamaters& p);
	[[nodiscard]] static BodyPartTypeId byName(std::string name);
	[[nodiscard]] static std::string& getName(const BodyPartTypeId& id);
	[[nodiscard]] static FullDisplacement getVolume(const BodyPartTypeId& id);
	[[nodiscard]] static bool getDoesLocamotion(const BodyPartTypeId& id);
	[[nodiscard]] static bool getDoesManipulation(const BodyPartTypeId& id);
	[[nodiscard]] static bool getVital(const BodyPartTypeId& id);
	[[nodiscard]] static std::vector<std::pair<AttackTypeId, MaterialTypeId>>& getAttackTypesAndMaterials(const BodyPartTypeId& id);
};
inline BodyPartType bodyPartTypeData;
// For example biped, quadraped, bird, etc.
class BodyType final
{
	StrongVector<std::string, BodyTypeId> m_name;
	StrongVector<std::vector<BodyPartTypeId>, BodyTypeId> m_bodyPartTypes;
public:
	static bool hasBodyPart(const BodyTypeId& id, const BodyPartTypeId& bodyPartType);
	static void create(std::string name, std::vector<BodyPartTypeId>& bodyPartTypes);
	[[nodiscard]] static BodyTypeId byName(std::string name);
	[[nodiscard]] static std::string getName(const BodyTypeId& id);
	[[nodiscard]] static std::vector<BodyPartTypeId>& getBodyPartTypes(const BodyTypeId& id);
};
inline BodyType g_bodyTypeData;
