#include "install.h"
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <algorithm>
#include "utils.h"

using json = nlohmann::json;
using namespace std;

json get_dependence_of_package(const string &package)
{
    // This function should return the dependencies of the package
    // For now, we will return an empty json object
    // In a real implementation, this would query the package manager or a database

    json dependencies;
    string cmd = "bash -c 'source pmp_venv/bin/activate && pip show " + package + " | grep Requires:'";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
    {
        cout << "pmp: Error getting dependencies for package: " << package << endl;
        return dependencies;
    }
    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe);
    // Remove the "Requires: " part
    size_t pos = result.find("Requires: ");
    if (pos != string::npos)
    {
        result = result.substr(pos + 10); // 10 is the length of "Requires: "
        // Split by comma and trim spaces
        stringstream ss(result);
        string item;
        while (getline(ss, item, ','))
        {
            item.erase(remove_if(item.begin(), item.end(), ::isspace), item.end()); // Trim spaces
            if (!item.empty())
            {
                dependencies[item] = {
                    {"version", get_installed_version(item)},
                    {"required_by", package} // Optional: you can add the package that requires this dependency
            }
        }
    }
    return dependencies;
}

string get_installed_version(const string &package)
{
    string cmd = "bash -c 'source pmp_venv/bin/activate && pip show " + package + " | grep ^Version:'";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return "";

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        result += buffer;
    }
    pclose(pipe);

    size_t pos = result.find("Version: ");
    if (pos != string::npos)
    {
        string version = result.substr(pos + 9);
        // Remove newline if exists
        if (!version.empty() && (version.back() == '\n' || version.back() == '\r'))
            version.pop_back();
        return version;
    }

    return "";
}

string get_base_package(const string &pkg)
{
    // Find the first occurrence of '[', '=', or '=='
    size_t pos = pkg.find_first_of("[=");
    if (pos == string::npos)
        return pkg;

    // For "==", we need to check it's not a single '=', since find_first_of may return '=' which is part of '=='
    if (pos > 0 && pkg[pos] == '=' && pos + 1 < pkg.size() && pkg[pos + 1] == '=')
        return pkg.substr(0, pos);

    return pkg.substr(0, pos);
}

string get_version(const string &pkg)
{
    // find the version with this regex: [==] example: "package==1.0.0"
    size_t pos = pkg.find("==");
    if (pos != string::npos)
    {
        return pkg.substr(pos + 2); // Return the version part after '=='
    }

    return "";
}

void install_dependencies(const json &config, json &lock)
{
}

void install_new_dependencies(json &config, json &lock, string &package)
{
    auto base_package = get_base_package(package);
    auto version = get_version(package);

    cout << "pmp: Installing package: " << base_package << endl;

    string command = "bash -c 'source pmp_venv/bin/activate && pip install " + base_package;
    if (!version.empty())
    {
        command += "==" + version;
    }
    command += "'";

    int resultado = std::system(command.c_str());

    string version_installed = get_installed_version(base_package);
    json dependencies = get_dependence_of_package(base_package);

    cout << "pmp: Package " << base_package << " installed with version: " << version_installed << endl;
    cout << "Dependencies of the package: " << dependencies.dump(4) << endl;
    cout << endl;

    // ✅ Verificación de tipos
    if (!config["dependencies"].is_object())
    {
        config["dependencies"] = json::object();
    }
    if (!config["dependencies_secundary"].is_object())
    {
        config["dependencies_secundary"] = json::object();
    }

    cout << "Tipo de config[\"dependencies\"]: " << config["dependencies"].type_name() << endl;
    cout << "Tipo de config[\"dependencies_secundary\"]: " << config["dependencies_secundary"].type_name() << endl;

    // ✅ Guardar dependencia principal
    config["dependencies"][base_package] = {
        {"version", version_installed},
        {"dependencies", dependencies}};

    // ✅ Guardar dependencias secundarias
    for (const auto &dep : dependencies)
    {
        config["dependencies_secundary"][dep] = {
            {"version", get_installed_version(dep)},
            {"dependencies", get_dependence_of_package(dep)}};
    }

    // Guardar archivo actualizado
    ofstream config_out("./pmp_config.json");
    if (!config_out.is_open())
    {
        cout << "pmp: Error saving pmp_config.json file.\n";
        return;
    }
    config_out << config.dump(4);
    config_out.close();
    cout << "pmp: pmp_config.json updated with package: " << base_package << endl;
}

void install(string package)
{
    cout << "pmp: Installing PMP dependency: " << package << endl;

    ifstream config_in("./pmp_config.json");
    if (!config_in.is_open())
    {
        cout << "pmp: pmp_config.json file not found.\n";
        return;
    }
    json config;
    config_in >> config;

    ifstream lock_in("./pmp_lock.json");
    if (!lock_in.is_open())
    {
        ofstream lock_create("./pmp_lock.json");
        lock_create << "[]";
        lock_create.close();
        lock_in.open("./pmp_lock.json");
    }
    json lock;
    lock_in >> lock;

    // Create virtual environment if it does not exist
    ifstream venv_check("./pmp_venv/bin/activate");
    if (!venv_check.is_open())
    {
        cout << "pmp: Creating virtual environment.\n";
        system("python3 -m venv pmp_venv");
    }

    if (package == "*")
    {
        return install_dependencies(lock, config);
    }
    return install_new_dependencies(lock, config, package);
}