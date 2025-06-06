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
using ordered_json = nlohmann::ordered_json;
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
                };
            }
        }
        return dependencies;
    }
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

void install_dependencies()
{
    // This function installs all dependencies listed in the pmp_config.json file in dependencies and dependencies_secundary
    cout << "pmp: Installing all dependencies from pmp_config.json..." << endl;

    ifstream config_in("./pmp_config.json");
    if (!config_in.is_open())
    {
        cout << "pmp: pmp_config.json file not found.\n";
        return;
    }
    json config;
    config_in >> config;
    config_in.close();

    ifstream lock_in("./pmp_lock.json");
    if (!lock_in.is_open())
    {
        cout << "pmp: pmp_lock.json file not found, creating a new one.\n";
        ofstream lock_create("./pmp_lock.json");
        lock_create << "[]";
        lock_create.close();
        lock_in.open("./pmp_lock.json");
    }
    json lock;
    lock_in >> lock;
    lock_in.close();

    // Create virtual environment if it does not exist
    ifstream venv_check("./pmp_venv/bin/activate");
    if (!venv_check.is_open())
    {
        cout << "pmp: Creating virtual environment.\n";
        system("python3 -m venv pmp_venv");
    }

    // Activate the virtual environment
    for (const auto &entry : config["dependencies"].items())
    {
        const string &package = entry.key();
        const string &version = entry.value()["version"];
        cout << "pmp: Installing package: " << package << " version: " << version << endl;

        string command = "bash -c 'source pmp_venv/bin/activate && pip install " + package;
        if (!version.empty())
        {
            command += "==" + version;
        }
        command += "'";

        int resultado = std::system(command.c_str());

        if (resultado != 0)
        {
            cout << "pmp: Error installing package: " << package << endl;
            continue;
        }

        // Add to lock file
        string version_installed = get_installed_version(package);
        if (!version_installed.empty())
        {
            lock.push_back(package + "==" + version_installed);
            cout << "pmp: Package installed and added to pmp_lock.json: " << package << endl;
        }
    }

    // Now we install the secondary dependencies
    for (const auto &entry : config["dependencies_secundary"].items())
    {
        const string &package = entry.key();
        const string &version = entry.value()["version"];
        cout << "pmp: Installing secondary package: " << package << " version: " << version << endl;
        string command = "bash -c 'source pmp_venv/bin/activate && pip install " + package;
        if (!version.empty())
        {
            command += "==" + version;
        }
        command += "'";
        int resultado = std::system(command.c_str());
        if (resultado != 0)
        {
            cout << "pmp: Error installing secondary package: " << package << endl;
            continue;
        }
    }

    // Save the updated lock file
    ofstream lock_out("./pmp_lock.json");
    if (!lock_out.is_open())
    {
        cout << "pmp: Error saving pmp_lock.json file.\n";
        return;
    }
    lock_out << lock.dump(4);
    lock_out.close();
}

void install_new_dependencies(string &package)
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

    ordered_json pmp_config;
    ifstream config_in("./pmp_config.json");

    config_in >> pmp_config;
    config_in.close();

    if (!pmp_config["dependencies"].is_object())
    {
        pmp_config["dependencies"] = json::object();
    }
    if (!pmp_config["dependencies_secundary"].is_object())
    {
        pmp_config["dependencies_secundary"] = json::object();
    }

    pmp_config["dependencies"][base_package] = {
        {"version", version_installed}};

    for (auto it = dependencies.begin(); it != dependencies.end(); ++it)
    {
        const string &dep_name = it.key();
        const json &dep_info = it.value();

        if (!pmp_config["dependencies_secundary"].contains(dep_name)){
            pmp_config["dependencies_secundary"][dep_name] = {
                {"version", dep_info["version"]},
                {"required_by", json::array()}};
        }

        if (!pmp_config["dependencies_secundary"][dep_name]["required_by"].is_array())
        {
            pmp_config["dependencies_secundary"][dep_name]["required_by"] = json::array();
        }

        for (const auto &required_by : dep_info["required_by"])
        {
            // Check if the base package is already in the required_by list
            auto &required_by_list = pmp_config["dependencies_secundary"][dep_name]["required_by"];
            if (std::find(required_by_list.begin(), required_by_list.end(), base_package) == required_by_list.end())
            {
                // If not, we add it
                required_by_list.push_back(base_package);
            }
        }


        //if (pmp_config["dependencies_secundary"].contains(dep_name))
        //{
        //    // If the dependency already exists, we update the version
        //    pmp_config["dependencies_secundary"][dep_name]["version"] = dep_info["version"];
        //    // and add the base package to the required_by list
        //    if (!pmp_config["dependencies_secundary"][dep_name]["required_by"].is_array())
        //    {
        //        pmp_config["dependencies_secundary"][dep_name]["required_by"] = json::array();
        //    }
        //    // Check if the base package is already in the required_by list
        //    auto &required_by = pmp_config["dependencies_secundary"][dep_name]["required_by"];
        //    if (std::find(required_by.begin(), required_by.end(), base_package) == required_by.end())
        //    {
        //        // If not, we add it
        //        required_by.push_back(base_package);
        //    }
        //}
        //else
        //{
        //    // If it does not exist, we add it
        //    pmp_config["dependencies_secundary"][dep_name] = {
        //        {"version", dep_info["version"]},
        //        {"required_by", {base_package}}};
        //}
    }

    // Ya al final agregamos el paquete base a dependencies_secundary también
    if (!pmp_config["dependencies_secundary"].contains(base_package))
    {
        pmp_config["dependencies_secundary"][base_package] = {
            {"version", version_installed},
            {"required_by", {base_package}}};
    }
    else
    {
        pmp_config["dependencies_secundary"][base_package]["version"] = version_installed;
        auto &required_by = pmp_config["dependencies_secundary"][base_package]["required_by"];
        if (!required_by.is_array())
            required_by = json::array();
        if (std::find(required_by.begin(), required_by.end(), base_package) == required_by.end())
            required_by.push_back(base_package);
    }

    // Save the updated config to pmp_config.json
    ofstream config_out("./pmp_config.json");

    if (!config_out.is_open())
    {
        cout << "pmp: Error saving pmp_config.json file.\n";
        return;
    }

    config_out << pmp_config.dump(4);
    config_out.close();
    cout << "pmp: pmp_config.json updated with package: " << base_package << endl;

    // now we update the lock file
    ordered_json pmp_lock;
    ifstream lock_in("./pmp_lock.json");
    if (lock_in.is_open())
    {
        lock_in >> pmp_lock;
    }
    else
    {
        cout << "pmp: pmp_lock.json file not found, creating a new one.\n";
    }
    lock_in.close();

    // Check if the package is already in the lock file
    bool found = false;
    for (const auto &entry : pmp_lock)
    {
        if (entry.is_string() && entry.get<string>().find(base_package + "==") == 0)
        {
            found = true;
            break;
        }
    }

    if (!found)
    {
        pmp_lock.push_back(base_package + "==" + version_installed);
    }

    ofstream lock_out("./pmp_lock.json");
    if (!lock_out.is_open())
    {
        cout << "pmp: Error saving pmp_lock.json file.\n";
        return;
    }

    lock_out << pmp_lock.dump(4);
    lock_out.close();
    cout << "pmp: pmp_lock.json updated with package: " << base_package << endl;
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
        install_dependencies();
        return;
    }
    install_new_dependencies(package);
    return;
}