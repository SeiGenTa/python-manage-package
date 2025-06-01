#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <algorithm>
#include "uninstall.h"

using json = nlohmann::json;
using namespace std;

void uninstall(string package) {
    cout << "pmp: Uninstalling PMP dependency: " << package << endl;

    // Load lock file
    ifstream pmp_lock("./pmp_lock.json");
    if (!pmp_lock.is_open()) {
        cout << "pmp: pmp_lock.json file not found.\n";
        return;
    }
    json lock;
    pmp_lock >> lock;

    // Load config file
    ifstream pmp_config("./pmp_config.json");
    if (!pmp_config.is_open()) {
        cout << "pmp: pmp_config.json file not found.\n";
        return;
    }
    json config;
    pmp_config >> config;

    // Helper to save JSON
    auto save_json = [](const string& path, const json& jdata) {
        ofstream out(path);
        if (out.is_open()) {
            out << jdata.dump(4);
            out.close();
            return true;
        }
        return false;
    };

    // Find and uninstall matching entries in lock
    bool found = false;
    if (lock.is_array()) {
        auto it = lock.begin();
        while (it != lock.end()) {
            if (it->is_string() && it->get<string>().rfind(package + "==", 0) == 0) {
                string fullpkg = it->get<string>();
                string cmd = "bash -c 'source pmp_venv/bin/activate && pip uninstall -y " + package + "'";
                int result = system(cmd.c_str());
                if (result == 0) {
                    cout << "pmp: Uninstalled " << fullpkg << " from virtual environment.\n";
                } else {
                    cout << "pmp: Failed to uninstall " << fullpkg << ".\n";
                }
                it = lock.erase(it);
                found = true;
            } else {
                ++it;
            }
        }
    }

    if (!found) {
        cout << "pmp: Package '" << package << "' not found in lock.\n";
    } else if (save_json("./pmp_lock.json", lock)) {
        cout << "pmp: Updated pmp_lock.json.\n";
    } else {
        cout << "pmp: Failed to update pmp_lock.json.\n";
    }

    // Remove from pmp_config.json
    if (config.contains("dependencies") && config["dependencies"].is_array()) {
        auto& deps = config["dependencies"];
        auto it = deps.begin();
        while (it != deps.end()) {
            if (it->is_string() && it->get<string>().rfind(package + "==", 0) == 0) {
                it = deps.erase(it);
            } else {
                ++it;
            }
        }
        if (save_json("./pmp_config.json", config)) {
            cout << "pmp: Updated pmp_config.json.\n";
        } else {
            cout << "pmp: Failed to update pmp_config.json.\n";
        }
    }
}
