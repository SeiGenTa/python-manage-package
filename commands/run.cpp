#include <iostream>
#include <fstream>
#include <cstring>
#include <nlohmann/json.hpp>

#include "run.h"

using json = nlohmann::json;

void run(char* arg) {
    std::ifstream file("./pmp_config.json");

    if (!file.is_open()) {
        std::cout << "pmp: pmp.json file not found.\n";
        return;
    }

    json config;
    file >> config;

    if (config.contains("functions") && config["functions"].is_object()){
        bool found = false;
        for (auto& [key, value] : config["functions"].items()) {
            if (value.is_string()) {
                
                if (strcmp(key.c_str(),arg) == 0) {
                    std::string command = value.get<std::string>();
                    std::string full_command = "bash -c 'source pmp_venv/bin/activate && " + command + "'";
                    std::system(full_command.c_str());
                    found = true;
                    continue;
                }
            } else {
                std::cout << "pmp: Function '" << key << "' is not a valid string.\n";
            }
        }
        if (!found) {
            std::cout << "pmp: Function '" << arg << "' not found in pmp.json.\n";
        }
    }
    else{
        std::cout << "pmp: Invalid pmp.json file format.\n";
        return;
    }
}