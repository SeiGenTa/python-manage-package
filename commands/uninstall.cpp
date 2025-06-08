#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <algorithm>
#include "uninstall.h"

using ordered_json = nlohmann::ordered_json;
using namespace std;

void uninstall(string package)
{
    cout << "pmp: Uninstalling package: " << package << endl;

    ordered_json pmp_config;

    ifstream config_in("./pmp_config.json");
    if (!config_in.is_open())
    {
        cout << "pmp: pmp_config.json file not found.\n";
        return;
    }
    config_in >> pmp_config;
    config_in.close();

    ordered_json pmp_lock;
    ifstream lock_in("./pmp_lock.json");
    if (!lock_in.is_open())
    {
        cout << "pmp: pmp_lock.json file not found.\n";
        return;
    }
    lock_in >> pmp_lock;
    lock_in.close();

    if (!pmp_config["dependencies"].is_object())
    {
        cout << "pmp: No dependencies found in pmp_config.json.\n";
        return;
    }

    if (!pmp_config["dependencies"].contains(package))
    {
        cout << "pmp: Package '" << package << "' not found in pmp_config.json.\n";
        return;
    }

    // Remove the package from dependencies
    pmp_config["dependencies"].erase(package);

    // We removed the dependency from those required
    for (auto &[dep_name, dep_info] : pmp_config["dependencies_secondary"].items())
    {
        if (dep_info.contains("required_by") && dep_info["required_by"].is_array())
        {
            auto &required_by = dep_info["required_by"];
            required_by.erase(
                std::remove(required_by.begin(), required_by.end(), package),
                required_by.end());
        }
    }

    // Remove from lock file all entries that start with 'package=='
    pmp_lock.erase(
        std::remove_if(
            pmp_lock.begin(),
            pmp_lock.end(),
            [&](const std::string &entry)
            {
                return entry.rfind(package + "==", 0) == 0;
            }),
        pmp_lock.end());

    // save the updated pmp_config.json
    ofstream config_out("./pmp_config.json");
    if (!config_out.is_open())
    {
        cout << "pmp: Error saving pmp_config.json file.\n";
        return;
    }
    config_out << pmp_config.dump(4);
    config_out.close();

    // save the updated pmp_lock.json
    ofstream lock_out("./pmp_lock.json");
    if (!lock_out.is_open())
    {
        cout << "pmp: Error saving pmp_lock.json file.\n";
        return;
    }
    lock_out << pmp_lock.dump(4);
    lock_out.close();

    uninstall_unused_dependencies();
}

void uninstall_unused_dependencies()
{
    // Check if pmp_config.json in the dependencies_secondary section has any dependencies
    // that are not used by any other package
    ordered_json pmp_config;
    ifstream config_in("./pmp_config.json");
    if (!config_in.is_open())
    {
        return;
    }
    config_in >> pmp_config;
    config_in.close();

    if (!pmp_config["dependencies_secondary"].is_object())
    {
        cout << "pmp: No secondary dependencies found in pmp_config.json.\n";
        return;
    }

    string command = "bash -c 'source pmp_venv/bin/activate && pip uninstall -y ";

    ordered_json &dep_sec = pmp_config["dependencies_secondary"];

    for (auto it = dep_sec.begin(); it != dep_sec.end();)
    {
        if (it.value().contains("required_by") && it.value()["required_by"].is_array())
        {
            // If the package is required by any other package, skip it
            if (!it.value()["required_by"].empty())
            {
                ++it;
                continue;
            }

            command += it.key() + " ";
            dep_sec.erase(it);
        }
    }

    // If there are no packages to uninstall, return
    if (command == "bash -c 'source pmp_venv/bin/activate && pip uninstall -y ")
    {
        return;
    }

    command += " >/dev/null 2>&1'";

    int result = std::system(command.c_str());

    if (result != 0)
    {
        cout << "pmp: Error uninstalling unused dependencies.\n";
        return;
    }
    cout << "pmp: Unused dependencies uninstalled successfully\n";

    // Save the updated config to pmp_config.json
    ofstream config_out("./pmp_config.json");
    if (!config_out.is_open())
    {
        cout << "pmp: Error saving pmp_config.json file.\n";
        return;
    }
    config_out << pmp_config.dump(4);
    config_out.close();
}
