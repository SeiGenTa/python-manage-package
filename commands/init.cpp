#include <iostream>
#include <fstream>
#include "init.h"

void init(){
    std::cout << "pmp: Initializing project...\n";

    // Check if the current directory is a valid project directory
    std::ifstream checkConfigFile("pmp_config.json");
    if (checkConfigFile.is_open()) {
        std::cout << "pmp: This directory is already a PMP project.\n";
        checkConfigFile.close();
        return;
    }

    // Create a virtual environment for the project
    int resultado = std::system("python3 -m venv pmp_venv");

    if (resultado == 0) {
        std::cout << "pmp: Project initialized successfully.\n";
        std::cout << "pmp: You can now start adding tasks and managing your project.\n";
    } else {
        std::cout << "pmp: Failed to initialize project.\n";
    }

    // Create a configuration file for the project
    std::ofstream configFile("pmp_config.json");
    std::ifstream templateConfigFile("/usr/local/share/pmp/templates/template_config.json");
    if (configFile.is_open() && templateConfigFile.is_open()) {
        std::string line;
        while (std::getline(templateConfigFile, line)) {
            configFile << line << "\n";
        }
        configFile.close();
        templateConfigFile.close();
        std::cout << "pmp: Configuration file created successfully.\n";
    } else {
        std::cout << "pmp: Failed to create configuration file.\n";
    }

    // Create a README file for the project
    std::ofstream readmeFile("README.md");
    if (readmeFile.is_open()) {
        readmeFile << "# PMP Project Management Tool\n";
        readmeFile << "This is a project managed by the PMP tool.\n";
        readmeFile << "## Getting Started\n";
        readmeFile << "To get started, run the following command:\n";
        readmeFile << "`python3 -m venv pmp_venv`\n";
        readmeFile.close();
        std::cout << "pmp: README file created successfully.\n";
    } else {
        std::cout << "pmp: Failed to create README file.\n";
    }

    // Create a main script file for the project
    std::ofstream mainFile("main.py");
    if (mainFile.is_open()) {
        mainFile << "import json\n";
        mainFile << "import os\n";
        mainFile << "\n";
        mainFile << "def main():\n";
        mainFile << "    print('Welcome to the PMP Project Management Tool!')\n";
        mainFile << "\n";
        mainFile << "if __name__ == '__main__':\n";
        mainFile << "    main()\n";
        mainFile.close();
        std::cout << "pmp: Main script file created successfully.\n";
    } else {
        std::cout << "pmp: Failed to create main script file.\n";
    }

    // Create gitignore file based in ../templates/template_gitignore
    std::ofstream gitignoreFile(".gitignore");

    //This 
    std::ifstream templateFile("/usr/local/share/pmp/templates/template_gitignore");
    if (gitignoreFile.is_open() && templateFile.is_open()) {
        std::string line;
        while (std::getline(templateFile, line)) {
            gitignoreFile << line << "\n";
        }
        gitignoreFile.close();
        templateFile.close();
        std::cout << "pmp: .gitignore file created successfully.\n";
    } else {
        std::cout << "pmp: Failed to create .gitignore file.\n";
    }

    // Create a lock file for the project
    std::ofstream lockFile("pmp_lock.json");

    std::ifstream pmpLockFile("/usr/local/share/pmp/templates/template_pmp_lock.json");

    if (lockFile.is_open() && pmpLockFile.is_open()) {
        std::string line;
        while (std::getline(pmpLockFile, line)) {
            lockFile << line << "\n";
        }
        lockFile.close();
        pmpLockFile.close();
        std::cout << "pmp: Lock file created successfully.\n";
    } else {
        std::cout << "pmp: Failed to create lock file.\n";
    }

}