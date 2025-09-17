#pragma once
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "../lib/json.hpp"
#pragma GCC diagnostic pop
using Json = nlohmann::json;

#include <bitset>

namespace nlohmann {
	template <size_t N>
	struct adl_serializer<std::bitset<N>> {
		static void to_json(json& j, const std::bitset<N>& b) {
			j = b.to_string();  // Store as string like "101"
		}

		static void from_json(const json& j, std::bitset<N>& b) {
			std::string bit_str = j.get<std::string>();
			assert(bit_str.length() == N);
			b = std::bitset<N>(bit_str);
		}
	};
}
