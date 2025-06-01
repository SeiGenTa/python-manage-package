#include <iostream>
#include <string>
#include "commands/init.h"
#include "commands/run.h"

int main(int argc, char* argv[]) {
    if (argc == 0) {
        std::cout << "pmp: You must specify a command.\n";
        std::cout << "Use 'pmp help' for more information.\n";
        return 1;
    }

    std::string comand = argv[1];

    if (comand == "init"){
        if (argc > 2) {
            std::cout << "pmp: Too many arguments for 'init' command.\n";
            return 1;
        }
        init();
    }

    if (comand == "run") {
        if (argc == 3){
            run(argv[2]);
            return 0;
        }
        char* defaultArg = "default";
        run(defaultArg); // Default argument if none is provided
    }


    return 0;
}
