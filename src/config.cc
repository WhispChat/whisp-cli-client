#include "whisp-cli/config.h"
#include "whisp-cli/logging.h"

#include <algorithm>
#include <mutex>
#include <vector>

namespace config {

const std::string config_filename = "user.cfg";
const std::unordered_map<std::string, std::string> default_config = {
    {"SERVER_HOST", "127.0.0.1"}, {"SERVER_PORT", "8080"}};

std::mutex config_fstream_mutex;

void load() {
  config_fstream_mutex.lock();
  std::fstream config_fstream(config_filename, std::ios::in | std::ios::out);

  if (!config_fstream) {
    // Create file and fill with default values in case it does not exist yet
    config_fstream.close();
    std::fstream config_fstream(config_filename, std::ios::out);

    for (std::pair<std::string, std::string> entry : default_config) {
      config_fstream << entry.first << " " << entry.second << std::endl;
    }
  } else {
    // If it already exists, verify all keys are present
    std::vector<std::string> f_keys;
    std::string f_key, f_value;
    while (config_fstream >> f_key >> f_value) {
      f_keys.push_back(f_key);
    }

    config_fstream.clear();
    for (std::pair<std::string, std::string> d_entry : default_config) {
      if (std::find(f_keys.begin(), f_keys.end(), d_entry.first) ==
          f_keys.end()) {
        config_fstream << d_entry.first << " " << d_entry.second << std::endl;
      }
    }
  }

  config_fstream.close();
  config_fstream_mutex.unlock();
}

std::string read(std::string lookup_key) {
  config_fstream_mutex.lock();
  std::fstream config_fstream(config_filename, std::ios::in | std::ios::out);
  std::string key, value;
  if (config_fstream.is_open()) {
    while (config_fstream >> key >> value) {
      if (key == lookup_key) {
        config_fstream.close();
        config_fstream_mutex.unlock();
        return value;
      }
    }
  }

  LOG_ERROR << "Could not find user configuration value for " << lookup_key
            << " - using default instead" << '\n';
  config_fstream.close();
  config_fstream_mutex.unlock();
  return default_config.find(lookup_key)->second;
}
} // namespace config