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
        for (auto& [key, value] : config["functions"].items()) {
            if (value.is_string()) {
                
                if (strcmp(key.c_str(),arg) == 0) {
                    std::string command = value.get<std::string>();
                    std::string full_command = "bash -c 'source pmp_venv/bin/activate && " + command + "'";
                    std::system(full_command.c_str());
                }
            } else {
                std::cout << "pmp: Function '" << key << "' is not a valid string.\n";
            }
        }
    }
    else{
        std::cout << "pmp: Invalid pmp.json file format.\n";
        return;
    }




}