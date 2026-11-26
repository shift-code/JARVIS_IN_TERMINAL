#pragma once

#include <string>

bool needsScreen(const std::string& text);

std::string askAboutScreen(const std::string& question,
                           const std::string& model = "qwen3-vl:2b",
                           int width = 800);
