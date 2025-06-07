#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <nlohmann/json.hpp>
#include "utils.h"

using json = nlohmann::json;
using namespace std;

json get_config()
{
    ifstream config_in("./pmp_config.json");
    if (!config_in.is_open())
    {
        cout << "pmp: pmp_config.json file not found.\n";
        return json();
    }
    json config;
    config_in >> config;
    return config;
}

json get_lock()
{
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
    return lock;
}

// Elimina espacios en blanco al inicio y final del string
string trim(const string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    size_t end = str.find_last_not_of(" \t\n\r\f\v");

    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

map<string,string> get_env_vars(string env_file)
{
    map<string, string> env_vars;
    ifstream env_in(env_file);
    if (!env_in.is_open())
    {
        cout << "pmp: Environment file not found: " << env_file << endl;
        return env_vars;
    }

    string line;
    while (getline(env_in, line))
    {
        size_t pos = line.find('=');
        if (pos != string::npos)
        {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);
            // remove the " " from the value if it exists
            value = trim(value);
            // remove the " " from the key if it exists
            key = trim(key);
            
            env_vars[key] = value;
        }
    }
    env_in.close();
    return env_vars;
}