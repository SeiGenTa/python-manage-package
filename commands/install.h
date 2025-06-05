#ifndef INSTALL_H
#define INSTALL_H

#include <string>
#include <nlohmann/json.hpp>

void install_dependencies(const nlohmann::json &config, nlohmann::json &lock);
void install_new_dependencies(const nlohmann::json &config, nlohmann::json &lock, const std::string &package);
std::string get_installed_version(const std::string &package);
std::string get_base_package(const std::string &pkg);
std::string get_version(const std::string &pkg);
nlohmann::json get_dependence_of_package(const std::string &package);
void install(std::string package);


#endif // INIT_H
