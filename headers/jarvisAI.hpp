#pragma once

#include <string>
#include "../nlohmann/json.hpp"

using json = nlohmann::json;

json askAI(const std::string& prompt);
