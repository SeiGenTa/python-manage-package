#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

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