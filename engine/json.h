#pragma once
// Some miscelanious serialization / deserialization fucnctions that don't belong anywhere else.

#include "../lib/dynamic_bitset.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include "../lib/json.hpp"
#pragma GCC diagnostic pop
using Json = nlohmann::json;

namespace nlohmann {
    template <>
    struct adl_serializer<sul::dynamic_bitset<>> {
        static void from_json(const json& data, sul::dynamic_bitset<>& bitset) {
            if (!data.is_string()) {
                throw std::invalid_argument("Invalid JSON format. Expected string.");
            }
            const std::string& str = data.get<std::string>();
            bitset.resize(str.size());
            for (size_t i = 0; i < str.size(); ++i) {
                if (str[i] == '0')
                    bitset[i] = false;
                else if (str[i] == '1')
                    bitset[i] = true;
                else
                    throw std::invalid_argument("Invalid character in JSON string.");
            }
        }

        static void to_json(json& data, const sul::dynamic_bitset<>& bitset) {
            std::string str;
            str.reserve(bitset.size());
            for (size_t i = 0; i < bitset.size(); ++i) {
                str.push_back(bitset[i] ? '1' : '0');
            }
            data = str;
        }
    };
}
