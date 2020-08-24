#ifndef PROJECT_SERVERPRIVATE_H
#define PROJECT_SERVERPRIVATE_H

// is this file really a good idea?

//#include "constants.h"
//#include "user.h"
//#include "server.h"

#define PROMPT ">" // !! REMOVE FROM HERE?

//#define WELCOME "____________________|                    \n|                    \n|                    \n|        WELCOME     \n|                    \n|                    \n|\n____________________"
#define WELCOMEFILE "welcome.txt"

void parseCredentials();

void freeUserList(UserList *userListR);

void parseArgs(int argc, char **argv, unsigned short *port); // argv const? got weird error/warning...

int buildSocket(unsigned short port);

char *readWelcome(void);

#endif //PROJECT_SERVERPRIVATE_H
