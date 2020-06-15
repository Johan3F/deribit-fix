#pragma once

#include <fstream>
#include <iostream>

#include <string>

#include <unordered_map>
#include <vector>

using config_file_t = std::unordered_map<std::string, std::string>;

namespace config_file {
/**
 * Loads a configurations file and return a map binding key and value
 */
inline config_file_t load_config_file(std::string filename) {
  std::ifstream input(filename);  // The input stream
  config_file_t return_map;
  while (input) {
    std::string key;                   // The key
    std::string value;                 // The value
    std::getline(input, key, ':');     // Read up to the : delimiter into key
    std::getline(input, value, '\n');  // Read up to the newline into value

    // Store the result in the map
    return_map[key] = value;
  }
  input.close();  // Close the file stream

  // Look for required keys
  std::vector<std::string> needed_keys{"AccessKey", "AccessSecret",
                                       "FIXConfigurationFile"};
  for (auto const &key : needed_keys) {
    if (return_map.find(key) == return_map.end()) {
      std::cerr << "User configuration file is missing the key: " << key
                << std::endl;
      config_file_t empty_map;
      return empty_map;
    }
  }

  return return_map;  // And return the result
}

}  // namespace config_file
