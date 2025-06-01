#include <iostream>
#include <fstream>
#include <cstring>
#include <nlohmann/json.hpp>

#include "run.h"

void run(char* arg) {
    if (std::strcmp(arg, "default") == 0) {
        int resultado = std::system("bash -c 'source pmp_venv/bin/activate && python3 main.py'");
        if (resultado != 0) {
            std::cout << "pmp: Failed to run the project.\n";
        }
        return;
    } else {
        std::cout << "pmp: Running with custom argument: " << arg << "\n";
    }
}