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

using json = nlohmann::json;
using namespace std;

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

string get_base_package(const string& pkg)
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
        if (!config.contains("dependencies") || !config["dependencies"].is_array())
        {
            cout << "pmp: Invalid or missing 'dependencies' in pmp_config.json.\n";
            return;
        }

        for (const auto &dep_entry : config["dependencies"])
        {
            if (!dep_entry.is_string())
            {
                cout << "pmp: Invalid dependency entry: " << dep_entry.dump() << "\n";
                continue;
            }

            string dep = dep_entry.get<string>();
            string base_package = get_base_package(dep);

            // Check if already in lock (with any version)
            bool already_installed = false;
            for (const auto &locked_dep : lock)
            {
                if (locked_dep.is_string())
                {
                    string locked_str = locked_dep.get<string>();
                    if (locked_str.rfind(base_package + "==", 0) == 0)
                    {
                        already_installed = true;
                        break;
                    }
                }
            }
            if (already_installed)
            {
                cout << "pmp: Package '" << base_package << "' is already installed.\n";
                continue;
            }

            // Install package with pip
            cout << "pmp: Installing package '" << dep << "'...\n";
            string install_cmd = "bash -c 'source pmp_venv/bin/activate && pip install \"" + dep + "\"'";
            int result = system(install_cmd.c_str());
            if (result != 0)
            {
                cout << "pmp: Failed to install '" << dep << "'. Skipping.\n";
                continue;
            }

            // Get installed version
            string version = get_installed_version(base_package);
            if (version.empty())
            {
                cout << "pmp: Failed to detect installed version for " << base_package << ". Skipping.\n";
                continue;
            }

            string pinned = base_package + "==" + version;

            // Add to lock if not already present (for safety)
            auto it_lock = std::find_if(lock.begin(), lock.end(), [&](const json& j){
                if (!j.is_string()) return false;
                return j.get<string>().rfind(base_package + "==", 0) == 0;
            });
            if (it_lock == lock.end())
                lock.push_back(pinned);

            // Update or add in config dependencies
            auto &deps = config["dependencies"];
            bool updated = false;
            for (auto& d : deps)
            {
                if (!d.is_string()) continue;
                string dstr = d.get<string>();
                string dbase = get_base_package(dstr);
                if (dbase == base_package)
                {
                    d = pinned;
                    updated = true;
                    break;
                }
            }
            if (!updated)
                deps.push_back(pinned);

            cout << "pmp: Installed and pinned " << pinned << "\n";
        }

        // Save updated lock and config
        ofstream lock_out("./pmp_lock.json");
        if (lock_out.is_open())
        {
            lock_out << lock.dump(4);
            lock_out.close();
            cout << "pmp: Updated pmp_lock.json.\n";
        }
        else
        {
            cout << "pmp: Failed to write pmp_lock.json.\n";
        }

        ofstream config_out("./pmp_config.json");
        if (config_out.is_open())
        {
            config_out << config.dump(4);
            config_out.close();
            cout << "pmp: Updated pmp_config.json.\n";
        }
        else
        {
            cout << "pmp: Failed to write pmp_config.json.\n";
        }

        return;
    }

    // Single package installation case
    string base_package = get_base_package(package);

    // Check if already installed (in lock)
    for (const auto &dep : lock)
    {
        if (dep.is_string() && dep.get<string>().rfind(base_package + "==", 0) == 0)
        {
            cout << "pmp: Package '" << base_package << "' is already installed.\n";
            return;
        }
    }

    // Install package with pip
    string install_cmd = "bash -c 'source pmp_venv/bin/activate && pip install \"" + package + "\"'";
    int result = system(install_cmd.c_str());
    if (result != 0)
    {
        cout << "pmp: Failed to install '" << package << "'.\n";
        return;
    }

    // Get installed version
    string version = get_installed_version(base_package);
    if (version.empty())
    {
        cout << "pmp: Failed to detect installed version.\n";
        return;
    }

    string pinned = base_package + "==" + version;

    // Add to lock and config dependencies
    lock.push_back(pinned);
    if (!config.contains("dependencies") || !config["dependencies"].is_array())
    {
        config["dependencies"] = json::array();
    }
    config["dependencies"].push_back(pinned);

    // Save lock and config
    ofstream lock_out("./pmp_lock.json");
    if (lock_out.is_open())
    {
        lock_out << lock.dump(4);
        lock_out.close();
    }
    else
    {
        cout << "pmp: Failed to write pmp_lock.json.\n";
    }

    ofstream config_out("./pmp_config.json");
    if (config_out.is_open())
    {
        config_out << config.dump(4);
        config_out.close();
    }
    else
    {
        cout << "pmp: Failed to write pmp_config.json.\n";
    }

    cout << "pmp: Package '" << pinned << "' installed and recorded.\n";
}
