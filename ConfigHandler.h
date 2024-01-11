//
// Created by dana on 11.01.2024.
//

#ifndef CONFIG_H
#define CONFIG_H


#include <iostream>
#include <string>
#include <libconfig.h++>

using namespace libconfig;


class ConfigHandler {
public:
  template<typename T>
  static T getConfigValue(const std::string &configPath, const std::string &groupName,
                          const std::string &settingName) {
    Config cfg;
    try {
      cfg.readFile(configPath.c_str());
    } catch ([[maybe_unused]] const FileIOException &fioex) {
      std::cerr << "I/O error while reading file." << std::endl;
      exit(EXIT_FAILURE);
    } catch (const ParseException &pex) {
      std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
              << " - " << pex.getError() << std::endl;
      exit(EXIT_FAILURE);
    }

    const Setting &root = cfg.getRoot();
    const Setting &group = root[groupName.c_str()];
    T value;
    try {
      if constexpr (std::is_same_v<T, std::string>) {
        value = group[settingName.c_str()].c_str();
      } else if constexpr (std::is_integral_v<T>) {
        value = group[settingName.c_str()];
      } else {
        throw std::runtime_error("Unsupported type");
      }
    } catch (const SettingNotFoundException &nfex) {
      std::cerr << "No '" << settingName << "' setting in configuration." << std::endl;
      exit(EXIT_FAILURE);
    }
    return value;
  }

};



#endif //CONFIG_H
