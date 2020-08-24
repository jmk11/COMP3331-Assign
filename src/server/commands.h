#ifndef PROJECT_COMMANDS_H
#define PROJECT_COMMANDS_H

#include "serverClients.h"
// !! horrible mess of #includes?

// bool (*matchCommand(char *command))(char *, Connection *);
bool execCommand(char *command, char *argsinput, Connection *connection);

#endif // PROJECT_COMMANDS_H
