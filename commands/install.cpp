#include "install.h"
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <set>
#include <algorithm>
#include "utils.h"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
using namespace std;

/**
 * Recursively retrieves all unique dependencies of a package in a flat JSON structure.
 *
 * Each dependency is listed only once, and the returned JSON maps each dependency name
 * to an object containing its installed version.
 *
 * @param package The root package to analyze.
 * @return A flat JSON object like: { "dep_name": { "version": "x.y.z" }, ... }
 */
json get_dependence_of_package(const string &package)
{
    // To avoid processing the same package multiple times
    set<string> visited;

    // Final JSON object to store dependencies and their versions
    json all_dependencies;

    // Recursive lambda function to resolve and add dependencies
    function<void(const string &)> resolve_dependencies = [&](const string &pkg)
    {
        if (visited.count(pkg))
            return; // Already processed

        visited.insert(pkg);

        // Build command to extract dependencies using pip
        string cmd = "bash -c 'source pmp_venv/bin/activate && pip show " + pkg + " | grep Requires:'";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            cerr << "pmp: Error getting dependencies for package: " << pkg << endl;
            return;
        }

        char buffer[128];
        string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            result += buffer;
        }
        pclose(pipe);

        // Parse line starting with "Requires: "
        size_t pos = result.find("Requires: ");
        if (pos != string::npos)
        {
            result = result.substr(pos + 10); // Strip the prefix
            stringstream ss(result);
            string item;
            while (getline(ss, item, ','))
            {
                // Trim spaces
                item.erase(remove_if(item.begin(), item.end(), ::isspace), item.end());

                if (!item.empty() && !visited.count(item))
                {
                    // Add dependency with version
                    all_dependencies[item] = {
                        {"version", get_installed_version(item)}};

                    // Recursively resolve dependencies of this dependency
                    resolve_dependencies(item);
                }
            }
        }
    };

    // Start from the root package (but do not include it in the result)
    resolve_dependencies(package);

    return all_dependencies;
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

    ordered_json pmp_config;
    ifstream config_in("./pmp_config.json");

    config_in >> pmp_config;
    config_in.close();

    // check if the package is already installed
    if (pmp_config["dependencies"].contains(base_package))
    {
        cout << "pmp: Package " << base_package << " is already installed.\n";
        return;
    }

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

        if (!pmp_config["dependencies_secundary"].contains(dep_name))
        {
            pmp_config["dependencies_secundary"][dep_name] = {
                {"version", dep_info["version"]},
                {"required_by", json::array()}};
        }

        if (!pmp_config["dependencies_secundary"][dep_name]["required_by"].is_array())
        {
            pmp_config["dependencies_secundary"][dep_name]["required_by"] = json::array();
        }

        auto &required_by_list = pmp_config["dependencies_secundary"][dep_name]["required_by"];

        required_by_list.push_back(base_package);
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