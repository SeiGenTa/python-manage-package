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
    cout << "Uninstalling package: " << package << endl;

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
    for (auto &[dep_name, dep_info] : pmp_config["dependencies_secundary"].items())
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

    cout << pmp_lock.dump(4) << endl;

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
    // Check if pmp_config.json in the dependencies_secundary section has any dependencies
    // that are not used by any other package
    ordered_json pmp_config;
    ifstream config_in("./pmp_config.json");
    if (!config_in.is_open())
    {
        return;
    }
    config_in >> pmp_config;
    config_in.close();

    if (!pmp_config["dependencies_secundary"].is_object())
    {
        return;
    }

    string command = "bash -c 'source pmp_venv/bin/activate && pip uninstall -y ";

    // Iterate through the secondary dependencies
    for (auto it = pmp_config["dependencies_secundary"].begin(); it != pmp_config["dependencies_secundary"].end();)
    {
        // Check if the package is used by any other package
        bool is_used = false;
        for (const auto &other_dep : pmp_config["dependencies_secundary"].items())
        {
            if (other_dep.key() == it.key())
                continue;

            if (other_dep.value().contains("required_by"))
            {
                for (const auto &depender : other_dep.value()["required_by"])
                {
                    if (depender == it.key())
                    {
                        is_used = true;
                        break;
                    }
                }
            }

            if (is_used)
                break;
        }

        // If not used, remove it
        if (!is_used)
        {
            command = command + it.key() + " ";
            it = pmp_config["dependencies_secundary"].erase(it);
        }
        else
        {
            ++it;
        }
    }

    // If there are packages to uninstall, execute the command
    if (command != "bash -c 'source pmp_venv/bin/activate && pip uninstall -y ")
    {
        command += "'";
        int result = std::system(command.c_str());
        if (result != 0)
        {
            return;
        }
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
}