#pragma once

#include "jarvis.hpp"

void listFiles(const std::string& dirPath = ".");
void writeFile(const std::string& filename, const std::string& content);
void readFile(const std::string& filename);
void deleteFile(const std::string& filename);
