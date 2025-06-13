#include <iostream>
#include <string>
#include "commands/init.h"
#include "commands/run.h"
#include "commands/install.h"
#include "commands/uninstall.h"
#include "commands/utils.h"

#ifdef _WIN32
const std::string SYSTEM = "Windows";
#else
const std::string SYSTEM = "Linux";
#endif

int main(int argc, char* argv[]) {
    if (argc == 0) {
        std::cout << "pmp: You must specify a command.\n";
        std::cout << "Use 'pmp help' for more information.\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "init"){
        if (argc > 2) {
            std::cout << "pmp: Too many arguments for 'init' command.\n";
            return 1;
        }
        init();
        return 0;
    }

    if (command == "run") {
        if (argc == 3){
            run(argv[2]);
            return 0;
        }
        char defaultArg[] = "*";
        run(defaultArg); // Default argument if none is provided
        return 0;
    }

    if (command == "install") {
        if (argc == 3) {
            std::string package = argv[2];
            install(package);
            return 0;
        }
        else if (argc == 2) {
            std::string package = "*"; // Default package if none is provided
            install(package); // Default package if none is provided   
            return 0;
        }
        return 1;
    }

    if (command == "uninstall") {
        if (argc == 3) {
            std::string package = argv[2];
            uninstall(package);
            return 0;
        }
        std::cout << "pmp: You must specify a package to uninstall.\n";
        return 1;
    }

    if (command == "clear"){
        // delete the pmp_lock.json file
        std::remove("pmp_lock.json");

        // and delete the folfer pmp_venv
        std::system("rm -rf pmp_venv");
        return 0;
    }

    if (command == "--version" || command == "-v") {
        if (SYSTEM == "Windows") {
            std::cout << "pmp version 0.4.2 (Windows)\n";
            return 0;
        }
        std::cout << "pmp version 0.4.2 (Linux)\n";
        return 0;
    }

    if (command == "help") {
        std::cout << "pmp: Project Management Tool\n";
        std::cout << "Available commands:\n";
        std::cout << "  init\t\t\tInitialize a new project\n";
        std::cout << "  run [arg]\t\tRun a command or script in the project environment\n";
        std::cout << "  install [pkg]\t\tInstall a package or dependency\n";
        std::cout << "  uninstall [pkg]\tUninstall a package or dependency\n";
        std::cout << "  clear\t\t\tClear the lock file and virtual environment\n";
        std::cout << "  --version, -v\t\tShow the version of pmp\n";
        return 0;
    }

    std::cout << "pmp: Unknown command '" << command << "'.\n";
    std::cout << "Use 'pmp help' for more information.\n";

    return 1;
}
