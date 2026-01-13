#include "psycologyData.h"
// Psycology Data
void PsycologyData::operator+=(const PsycologyData& other) { m_data += other.m_data; }
void PsycologyData::operator-=(const PsycologyData& other) { m_data -= other.m_data; }
void PsycologyData::operator*=(const float& scale) { m_data = m_data.cast<float>() * scale; }
PsycologyData PsycologyData::operator-(const PsycologyData& other) const { return PsycologyData(m_data - other.m_data); }
void PsycologyData::addTo(const PsycologyAttribute& attribute, const PsycologyWeight& value) { m_data[(int)attribute] += value.get(); }
void PsycologyData::set(const PsycologyAttribute& attribute, const PsycologyWeight& value) { m_data[(int)attribute] = value.get(); }
void PsycologyData::setAllToZero() { m_data = 0; }
const PsycologyWeight PsycologyData::getValueFor(const PsycologyAttribute& attribute) const { return {m_data[(int)attribute]}; }
bool PsycologyData::anyAbove(const PsycologyData& other) const { return (m_data > other.m_data).any(); }
bool PsycologyData::anyBelow(const PsycologyData& other) const { return (m_data < other.m_data).any(); }
SmallSet<PsycologyAttribute> PsycologyData::getAbove(const PsycologyData& other) const
{
	const auto result = m_data > other.m_data;
	SmallSet<PsycologyAttribute> output;
	for(int i = 0; i < (int)PsycologyAttribute::Null; ++i)
		if(result[i])
			output.insert((PsycologyAttribute)i);
	return output;
}
[[nodiscard]] SmallSet<PsycologyAttribute> PsycologyData::getBelow(const PsycologyData& other) const
{
	const auto result = m_data < other.m_data;
	SmallSet<PsycologyAttribute> output;
	for(int i = 0; i < (int)PsycologyAttribute::Null; ++i)
		if(result[i])
			output.insert((PsycologyAttribute)i);
	return output;
}