#pragma once

#include <fstream>
#include <unordered_map>
#include <string>

namespace config {
extern const std::string config_filename;
extern const std::unordered_map<std::string, std::string> config_default_vars;

extern std::fstream config_fstream;

void load();
std::string read(std::string lookup_key);

}; // namespace Config
