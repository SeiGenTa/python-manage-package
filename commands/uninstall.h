#ifndef COMMANDS_UNINSTALL_H
#define COMMANDS_UNINSTALL_H

#include <string>
using namespace std;

void uninstall(string package);
void uninstall_unused_dependencies();

#endif //COMMANDS_UNINSTALL_H