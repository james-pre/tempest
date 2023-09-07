#ifndef H_utils
#define H_utils

#include <nlohmann/json.hpp>
using json = nlohmann::json;

bool isValidJSON(const std::string& content) {
    try {
#pragma GCC diagnostic ignored "-Wunused-result"
        json::parse(content);
        return true;
    } catch (...) {
        return false;
    }
}

template <typename T>
json serializableToJson(const T& object)
{
    typename T::Serialized serialized = object.serialize();
    json jsonForm = serialized;
    return jsonForm;
}

template <typename T>
T serializableFromJson(const json& jsonData)
{
    typename T::Serialized serialized;
    jsonData.get_to(serialized);
    return T::deserialize(serialized);
}

#endif