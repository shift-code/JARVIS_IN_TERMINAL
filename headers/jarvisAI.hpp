#pragma once
#include "jarvis.hpp"

using json = nlohmann::json;

json askAI(const std::string& prompt);
