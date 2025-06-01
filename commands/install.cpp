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

string get_installed_version(const string& package) {
    string cmd = "bash -c 'source pmp_venv/bin/activate && pip show " + package + " | grep ^Version:'";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buffer[128];
    string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    size_t pos = result.find("Version: ");
    if (pos != string::npos) {
        return result.substr(pos + 9);  // 9 = length of "Version: "
    }

    return "";
}

void install(string package) {
    cout << "pmp: Installing PMP dependencies...: " << package << endl;

    ifstream pmp_config("./pmp_config.json");
    if (!pmp_config.is_open()) {
        cout << "pmp: pmp_config.json file not found.\n";
        return;
    }

    ifstream pmp_lock("./pmp_lock.json");
    if (!pmp_lock.is_open()) {
        cout << "pmp: pmp_lock.json file not found. Creating a new one...\n";
        ofstream pmp_lock_out("./pmp_lock.json");
        if (pmp_lock_out.is_open()) {
            pmp_lock_out << "[]";
            pmp_lock_out.close();
            cout << "pmp: pmp_lock.json file created successfully.\n";
        } else {
            cout << "pmp: Failed to create pmp_lock.json file.\n";
            return;
        }
        pmp_lock.open("./pmp_lock.json");
    }

    ifstream venv_check("./pmp_venv/bin/activate");
    if (!venv_check.is_open()) {
        cout << "pmp: pmp_venv directory not found. making.\n";
        int result = system("python3 -m venv pmp_venv");
        if (result != 0) {
            cout << "pmp: Failed to create virtual environment 'pmp_venv'.\n";
            return;
        }
    }

    json config;
    pmp_config >> config;
    json lock;
    pmp_lock >> lock;

    auto save_json = [](const string& path, const json& jdata) {
        ofstream out(path);
        if (out.is_open()) {
            out << jdata.dump(4);
            out.close();
            return true;
        }
        return false;
    };

    auto package_installed = [&](const string& fullpkg) {
        return any_of(lock.begin(), lock.end(), [&](const string& item) {
            return item == fullpkg;
        });
    };

    if (package != "*") {
        // Install package
        cout << "pmp: Installing package '" << package << "'...\n";
        string pip_cmd = "bash -c 'source pmp_venv/bin/activate && pip install " + package + "'";
        if (system(pip_cmd.c_str()) != 0) {
            cout << "pmp: Failed to install package '" << package << "'.\n";
            return;
        }

        string version = get_installed_version(package);
        version.erase(remove(version.begin(), version.end(), '\n'), version.end());
        if (version.empty()) {
            cout << "pmp: Failed to determine installed version of '" << package << "'.\n";
            return;
        }

        string fullpkg = package + "==" + version;

        if (!package_installed(fullpkg)) {
            lock.push_back(fullpkg);
            save_json("./pmp_lock.json", lock);
            cout << "pmp: Updated pmp_lock.json with '" << fullpkg << "'.\n";
        }

        if (config.contains("dependencies") && config["dependencies"].is_array()) {
            config["dependencies"].push_back(fullpkg);
            save_json("./pmp_config.json", config);
            cout << "pmp: Updated pmp_config.json with '" << fullpkg << "'.\n";
        }

        return;
    }

    // "*" case: install all from pmp_config
    if (config.contains("dependencies") && config["dependencies"].is_array()) {
        for (const auto& dep : config["dependencies"]) {
            if (!dep.is_string()) continue;
            string depstr = dep.get<string>();
            if (package_installed(depstr)) {
                cout << "pmp: Package '" << depstr << "' already installed.\n";
                continue;
            }

            string pip_cmd = "bash -c 'source pmp_venv/bin/activate && pip install " + depstr + "'";
            cout << "pmp: Installing '" << depstr << "'...\n";
            if (system(pip_cmd.c_str()) == 0) {
                lock.push_back(depstr);
                cout << "pmp: Installed '" << depstr << "'.\n";
            } else {
                cout << "pmp: Failed to install '" << depstr << "'.\n";
            }
        }
        save_json("./pmp_lock.json", lock);
    }
}
