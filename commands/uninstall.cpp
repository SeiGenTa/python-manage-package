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

    ifstream lock_in("./pmp_lock.json");
    ifstream config_in("./pmp_config.json");
    if (!lock_in.is_open() || !config_in.is_open()) {
        cout << "pmp: Required files not found.\n";
        return;
    }

    json lock, config;
    lock_in >> lock;
    config_in >> config;

    auto match_pkg = [&](const string& entry) {
        return entry.rfind(package + "==", 0) == 0;
    };

    bool removed = false;

    // Search in lock
    auto lock_it = lock.begin();
    while (lock_it != lock.end()) {
        if (lock_it->is_string() && match_pkg(lock_it->get<string>())) {
            string full = lock_it->get<string>();
            string uninstall_cmd = "bash -c 'source pmp_venv/bin/activate && pip uninstall -y " + package + "'";
            system(uninstall_cmd.c_str());
            lock_it = lock.erase(lock_it);
            removed = true;
        } else {
            ++lock_it;
        }
    }

    // Search in config
    if (config.contains("dependencies")) {
        auto& deps = config["dependencies"];
        auto dep_it = deps.begin();
        while (dep_it != deps.end()) {
            if (dep_it->is_string() && match_pkg(dep_it->get<string>())) {
                dep_it = deps.erase(dep_it);
            } else {
                ++dep_it;
            }
        }
    }

    if (removed) {
        ofstream lock_out("./pmp_lock.json");
        lock_out << lock.dump(4);
        lock_out.close();

        ofstream config_out("./pmp_config.json");
        config_out << config.dump(4);
        config_out.close();

        cout << "pmp: Uninstalled and cleaned up '" << package << "'.\n";
    } else {
        cout << "pmp: Package '" << package << "' not found in lock.\n";
    }
}
