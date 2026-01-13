#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "../lib/json.hpp"
#pragma GCC diagnostic pop
using Json = nlohmann::json;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#include "../lib/Eigen/Dense"
#pragma GCC diagnostic pop

#include <bitset>
namespace nlohmann {
	template <size_t N>
	struct adl_serializer<std::bitset<N>> {
		static void to_json(Json& j, const std::bitset<N>& b)
		{
			j = b.to_string();  // Store as string like "101"
		}
		static void from_json(const Json& j, std::bitset<N>& b)
		{
			std::string bit_str = j.get<std::string>();
			assert(bit_str.length() == N);
			b = std::bitset<N>(bit_str);
		}
	};
	// Eigen array holding a 1d series of float.
	template <int32_t capacity>
	struct adl_serializer<Eigen::Array<float, 1, capacity>>
	{
		static void to_json(Json& j, const Eigen::Array<float, 1, capacity>& v)
		{
			j = Json::array();
			j.get_ptr<json::array_t*>()->reserve(capacity);
			for(const float& f : v)
				j.push_back(f);
		}
		static void from_json(const Json& j, Eigen::Array<float, 1, capacity>& v)
		{
			for(int i = 0; i != capacity; ++i)
				v[i] = j[i];
		}
	};
}
