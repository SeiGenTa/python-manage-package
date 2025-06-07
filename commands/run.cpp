#include <iostream>
#include <fstream>
#include <string>
#include <nlohmann/json.hpp>
#include <map>

using json = nlohmann::json;

#include "utils.h"
#include "run.h"

void run(char *arg)
{
    std::ifstream file("./pmp_config.json");

    if (!file.is_open())
    {
        std::cout << "pmp: pmp.json file not found.\n";
        return;
    }

    json config;
    file >> config;

    std::map<string, string> env_vars;

    if (config.contains("env_file") || config["env_file"].is_string())
    {
        std::string env_file = config["env_file"].get<std::string>();
        env_vars = get_env_vars(env_file);
    }

    if (config.contains("functions") && config["functions"].is_object())
    {
        bool found = false;
        for (auto &[key, value] : config["functions"].items())
        {
            if (value.is_string())
            {

                if (strcmp(key.c_str(), arg) == 0)
                {
                    std::string command = value.get<std::string>();
                    // Replace environment variables in the command
                    for (const auto &[env_key, env_value] : env_vars)
                    {
                        size_t pos = command.find("{" + env_key + "}");
                        while (pos != std::string::npos)
                        {
                            command.replace(pos, env_key.length() + 2, env_value);
                            pos = command.find("{" + env_key + "}", pos + env_value.length()); // 👈 avanzar
                        }
                    }

                    std::string full_command = "bash -c 'source pmp_venv/bin/activate && " + command + "'";
                    std::system(full_command.c_str());
                    found = true;
                    continue;
                }
            }
            else
            {
                std::cout << "pmp: Function '" << key << "' is not a valid string.\n";
            }
        }
        if (!found)
        {
            std::cout << "pmp: Function '" << arg << "' not found in pmp.json.\n";
        }
    }
    else
    {
        std::cout << "pmp: Invalid pmp.json file format.\n";
        return;
    }
}