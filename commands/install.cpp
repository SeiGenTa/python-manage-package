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

    // Get the base package name (without extras or version constraints)
    string base_package = get_base_package(package);

    // Recursive lambda function to resolve and add dependencies
    function<void(const string &)> resolve_dependencies = [&](const string &pkg)
    {
        // Get the base package name without extras or version constraints
        string pkg_base = get_base_package(pkg);
        
        if (visited.count(pkg_base))
            return; // Already processed

        visited.insert(pkg_base);

        // Build command to extract dependencies using pip
        string cmd = "bash -c 'source pmp_venv/bin/activate && pip show " + pkg_base + " | grep \"^Requires:\\|^Requires-Dist:\" 2>/dev/null'";
        FILE *pipe = popen(cmd.c_str(), "r");
        if (!pipe)
        {
            cerr << "pmp: Error getting dependencies for package: " << pkg_base << endl;
            return;
        }

        char buffer[512]; // Increased buffer size for packages with many dependencies
        string result;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
        {
            result += buffer;
        }
        
        int exit_status = pclose(pipe);
        if (exit_status != 0)
        {
            cerr << "pmp: Command failed with exit status " << exit_status << " for package: " << pkg_base << endl;
            return;
        }

        // Parse lines starting with "Requires: " or "Requires-Dist: " (different pip versions)
        stringstream ss(result);
        string line;
        
        while (getline(ss, line))
        {
            size_t pos = line.find("Requires: ");
            if (pos == string::npos)
            {
                pos = line.find("Requires-Dist: ");
                if (pos != string::npos)
                {
                    line = line.substr(pos + 14); // Strip the "Requires-Dist: " prefix
                }
            }
            else
            {
                line = line.substr(pos + 10); // Strip the "Requires: " prefix
            }
            
            if (pos != string::npos) // Found a requirements line
            {
                stringstream deps_stream(line);
                string item;
                
                while (getline(deps_stream, item, ','))
                {
                    // Trim spaces and other whitespace
                    item = trim(item);
                    
                    // Extract the base package name (without extras or version constraints)
                    string dep_base = get_base_package(item);
                    
                    if (!dep_base.empty() && !visited.count(dep_base))
                    {
                        // Get the installed version
                        string version = get_installed_version(dep_base);
                        
                        // Only add if the package is actually installed
                        if (!version.empty())
                        {
                            // Add dependency with version
                            all_dependencies[dep_base] = {
                                {"version", version}
                            };
                            
                            // Recursively resolve dependencies of this dependency
                            resolve_dependencies(dep_base);
                        }
                        else
                        {
                            cerr << "pmp: Warning - Dependency " << dep_base << " is required by " 
                                 << pkg_base << " but not installed." << endl;
                        }
                    }
                }
            }
        }
    };

    // Start from the root package (but do not include it in the result)
    resolve_dependencies(base_package);

    return all_dependencies;
}

string get_installed_version(const string &package)
{
    string cmd = "bash -c 'source pmp_venv/bin/activate && pip show " + package + " | grep ^Version: 2>/dev/null'";
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
    // This function installs all dependencies listed in the pmp_config.json file in dependencies and dependencies_secondary
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
        system("python3 -m venv pmp_venv >/dev/null 2>&1");
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
        command += " >/dev/null 2>&1'";

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
            cout << "pmp: " << package << "==" << version_installed << " has installed correctly" << endl;
        }
    }

    // Now we install the secondary dependencies
    for (const auto &entry : config["dependencies_secondary"].items())
    {
        const string &package = entry.key();
        const string &version = entry.value()["version"];
        cout << "pmp: Installing secondary package: " << package << " version: " << version << endl;
        string command = "bash -c 'source pmp_venv/bin/activate && pip install " + package;
        if (!version.empty())
        {
            command += "==" + version;
        }
        command += " >/dev/null 2>&1'";
        int resultado = std::system(command.c_str());
        if (resultado != 0)
        {
            cout << "pmp: Error installing secondary package: " << package << endl;
            continue;
        }
        cout << "pmp: " << package << " has installed correctly as secondary dependency" << endl;
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
    command += " >/dev/null 2>&1'";

    int resultado = std::system(command.c_str());

    if (resultado == 0) {
        cout << "pmp: " << base_package << " has installed correctly" << endl;
    }

    string version_installed = get_installed_version(base_package);
    json dependencies = get_dependence_of_package(base_package);

    if (!pmp_config["dependencies"].is_object())
    {
        pmp_config["dependencies"] = json::object();
    }
    if (!pmp_config["dependencies_secondary"].is_object())
    {
        pmp_config["dependencies_secondary"] = json::object();
    }

    pmp_config["dependencies"][base_package] = {
        {"version", version_installed}};

    // Use range-based for loop for cleaner iteration
    for (const auto &[dep_name, dep_info] : dependencies.items())
    {
        // Get a reference to the dependency entry, creating it if needed
        auto &dep_entry = pmp_config["dependencies_secondary"][dep_name];

        // Initialize or update the version
        dep_entry["version"] = dep_info["version"];

        // Ensure required_by is an array
        if (!dep_entry.contains("required_by") || !dep_entry["required_by"].is_array())
        {
            dep_entry["required_by"] = json::array();
        }

        // Add the base package to the required_by list
        dep_entry["required_by"].push_back(base_package);
    }

    // Add the base package to dependencies_secondary as well
    auto &base_dep_entry = pmp_config["dependencies_secondary"][base_package];

    // Update version
    base_dep_entry["version"] = version_installed;

    // Ensure required_by is an array
    if (!base_dep_entry.contains("required_by") || !base_dep_entry["required_by"].is_array())
    {
        base_dep_entry["required_by"] = json::array();
    }

    // Add the base package to its own required_by list if not already present
    auto &required_by = base_dep_entry["required_by"];
    if (std::find(required_by.begin(), required_by.end(), base_package) == required_by.end())
    {
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
        system("python3 -m venv pmp_venv >/dev/null 2>&1");
    }

    if (package == "*")
    {
        install_dependencies();
        return;
    }
    install_new_dependencies(package);
    return;
}
