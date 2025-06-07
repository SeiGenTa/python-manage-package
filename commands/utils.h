#ifndef UTILS_H
#define UTILS_H

#include <nlohmann/json.hpp>
#include <map>

using namespace std;

using json = nlohmann::json;

// Get configuration from pmp_config.json
json get_config();

// Get lock file content from pmp_lock.json
// Creates the file if it doesn't exist
json get_lock();

map<string,string> get_env_vars(string env_file);

string trim(const string& str);

#endif // UTILS_H
