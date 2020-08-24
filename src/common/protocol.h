#ifndef PROJECT_PROTOCOL_H
#define PROJECT_PROTOCOL_H

#include "constants.h"

char *parseCommand(char *input, char **args);
void parseRequest(char *input, int numArgs, char *words[numArgs]);
unsigned int textToProtocol(char *input);

//unsigned int addrToProtocol(uint32_t ipaddr, unsigned short port, char *output);
//unsigned int addrToProtocol(uint32_t ipaddr, unsigned short port, const char *username, char **output);
unsigned int addrToProtocol(uint32_t ipaddr, unsigned short port, const char *requesteeName, const char *requesterName, char **output);
void freeProtocolStr(char *str);
//char *protocolToAddr(uint32_t *ipaddr, unsigned short *port, char **username, char *input);
char *protocolToAddr(uint32_t *ipaddr, unsigned short *port, char **partnerName, char **myName, char *input);
unsigned int portToProtocol(unsigned short port, char **output);

#define PRINT (-1)
#define LOGIN (-2) //255 // 0b11111111
// !! thing I wanted to check compilation of: char a = -1; if (a == 255) {}. inspired by former client.c comparison to LOGIN
//          Answer: When the comparison is not compiled out, the -1 is loaded into register with sign extension and the 255 with zero extension -> different
// in theory, could fit first byte of buffer with 127? different commands
#define P2P (-3)
#define P2PFAIL (-4)



/* OLD
 * typedef struct Argument Argument;
struct Argument
{
    char *string;
    Argument *next;
};

void freeArgs(Argument *args);
char *popArg(Argument **headP);
*/

#endif //PROJECT_PROTOCOL_H
