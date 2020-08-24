/*
Provides functions to convert to and from communication protocol
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"

// could format as text
// or as binary data of a struct? crazy?

// FORMAT:
// <Command>
// <InfoType>: <Info>
// (OR NOT)

// ALLOWED COMMANDS:
// message          <username>      <message>
// broadcast        <message>
// whoelse
// whoelsesince      <time>
// block            <user>
// unblock          <user>
// logout

// startprivate
// private
// stopprivate

// ALLOWED INFO TYPES:
// User
// Message
// Time

// Client send request as command0arg10arg20
// or command000
// or command0arg100

char *parseCommand(char *input, char **args) {
    *args = input + strlen(input) + 1; // skip null byte
    //if (*args[0] == 0) { return NULL; } // double null byte -> no args
    return input;
}

// from protocol
// call parseProtocol? Or ... ?
// NO! can be unlimited args because message is passed as args. // ! This is fine because there NEED to be unlimited args for messages
// probably need linked list again..........
// no! I can just change protocol to not null terminate the words in the message
// retrieve ARGS
// input points to first arg
/*
 * !! multiWordLast: is the last word a multiword !!
 */
void parseRequest(char *input, int numArgs, char *words[numArgs]) {
    // null-terminate first word
    // null-terminate at spaces up to last arg. Leave it as multi-words.
    // really, splits args into numArgs, with the first numArgs-1 single words and the last many words
    // name?

    for (int i = 0; i < numArgs; i++) {
        words[i] = NULL;
    }

    if (*input != 0) { // else keep args as null
        words[0] = input;

        unsigned char wordCount = 1;
        for (char *cur = input; (!(cur[0] == 0 && cur[1] == 0)) && (wordCount < numArgs); cur++) {
            if (*cur == ' ') {
                *cur = 0;
                words[wordCount] = cur + 1;
                wordCount++;
            }
        }
    }
}

// to Protocol
// asumming space available for extra null byte after input
// <WORDONE>0<WORDTWO>0<MULTIWORDS>00
// still doesn't work because with broadcast, multi-word message needs to be the second word
// ONLY in the case of a certain command, I want the second word to be multi-words
// maybe send command to server, and THEN after it responds with info on args, send args?
unsigned int textToProtocol(char *input) {
    unsigned int i;
    unsigned char numWords = 0;
    for (i = 0; input[i] != 0 && numWords < 1; i++) // numWords < 1 this doesn't need to be a loop anymore !!
    {
        if (input[i] == ' ') {
            input[i] = 0;
            numWords++; // end of word
        }
        // after 2 words ended, continue until null byte, and then add an extra null byte
        // CHANGED TO ONE WORD
    }
    for (; input[i] != 0; i++) {
    }                 // to avoid testing numWords < 1 on every loop
    input[i + 1] = 0; // finishes with two zeroes
    return i + 2;     // number of bytes to transmit - includes null bytes
}

// output must have at least 7 bytes available
// 0 means no bytes put to output - size too small
// output will be filled with malloced memory
unsigned int addrToProtocol(uint32_t ipaddr, unsigned short port, const char *requesteeName, const char *requesterName, char **output) // why am I using uint32 instead of unsigned int, but still using short???
{
    unsigned int requiredSize = sizeof(ipaddr) + sizeof(port) + strlen(requesteeName) + 1 + strlen(requesterName) + 1;
    //if (outputSize < requiredSize) { return 0; }
    *output = Malloc(requiredSize);
    unsigned int addSize;
    unsigned int cumulativeSize = 0;

    addSize = sizeof(ipaddr);
    memcpy(*output, &ipaddr, addSize); // this is getting weird
    cumulativeSize += addSize;

    addSize = sizeof(port);
    memcpy((*output) + cumulativeSize, &port, addSize);
    cumulativeSize += addSize;

    strcpy((*output) + cumulativeSize, requesteeName);
    cumulativeSize += strlen(requesteeName) + 1;

    strcpy((*output) + cumulativeSize, requesterName);

    return requiredSize - 1; // number of bytes to append to buffer - does not include null byte
    // there are some real inconsistency problems here
}

void freeProtocolStr(char *str) {
    free(str);
}

char *protocolToAddr(uint32_t *ipaddr, unsigned short *port, char **partnerName, char **myName, char *input) {
    unsigned int addSize = sizeof(*ipaddr);
    memcpy(ipaddr, input, addSize);
    unsigned int cumulativeSize = addSize;

    addSize = sizeof(*port);
    memcpy(port, input + cumulativeSize, addSize);
    cumulativeSize += addSize;

    *partnerName = input + cumulativeSize; // everything is bad now
    cumulativeSize += strlen(*partnerName) + 1;

    *myName = input + cumulativeSize;
    cumulativeSize += strlen(*myName);

    return input + cumulativeSize; // points to last null byte
}

// caller must call freeProtocolStr(portMessage) once finished with message
unsigned int portToProtocol(unsigned short port, char **output) {
    char *portCommand = "setupp2p"; // doesn't make sense to have this here - none of the other text commands are in this file
    unsigned int portCommandLength = strlen(portCommand);
    unsigned int requiredSize = portCommandLength + 1 + sizeof(port) + 2;
    //if (outputSize < requiredSize) { return 0; }
    *output = Malloc(requiredSize);
    memcpy(*output, portCommand, portCommandLength);
    (*output)[portCommandLength] = 0;
    memcpy((*output) + portCommandLength + 1, &port, sizeof(port));
    (*output)[portCommandLength + 1 + sizeof(port)] = 0;
    (*output)[portCommandLength + 1 + sizeof(port) + 1] = 0; // double null byte termination
    return requiredSize;                                     // number of bytes to transmit, includes null bytes
    // this is all so inconsistent
}

/*
void parseRequest(char *input, int numArgs, char *words[numArgs])
{
    for (int i = 0; i < numArgs; i++) {
        words[i] = NULL;
    }

    if (*input != 0) { // else keep args as null
        words[0] = input;

        unsigned char wordCount = 1;
        for (char *cur = input; !(cur[0] == 0 && cur[1] == 0); cur++) {
            // twice checking if cur[0] == 0. prob would be optimised out easily
            if (*cur == 0) {
                if (wordCount < numArgs) {
                    words[wordCount] = cur + 1;
                    wordCount++;
                }
                else {
                    *cur = ' '; // change all null bytes to spaces, except when there are two in the row at the end
                }
            }
        }
    }
}*/

/*
void textToCommand(char *input)
{
    // !! NULLs
    // #defines?
    // should the client know the allowed messages or better just determined at server?
    // I think better just determined at server. Maybe should just send client's raw input to server
    // so client doesn't need to be updated if commands change

    // this is an awful implementation
    // if commands were changed so there could be 4 commands, the struct would have to be changed as would every one of these ifs


    char *textCommand = input;
    // char *args = getArgs(input);
    char *args = textCommand + strlen(textCommand) + 1;

    struct Command2 command; // NAMES
    if (strcmp(textCommand, "message") == 0)
    {
        command.command = MESSAGE;
        command.args = args;
    }
    else if (strcmp(request.command, "broadcast") == 0)
    {
        command.command = BROADCAST;
        command.arg1 = request.arg1;
        command.arg2 = NULL;
    }
    else if (strcmp(request.command, "whoelse") == 0)
    {
        command.command = WHOELSE;
        command.arg1 = NULL;
        command.arg2 = NULL;
    }
    else if (strcmp(request.command, "whoelsesince") == 0)
    {
        command.command = MESSAGE;
        command.arg1 = request.arg1;
        command.arg2 = request.arg2;
    }
    else if (strcmp(request.command, "block") == 0)
    {
        command.command = MESSAGE;
        command.arg1 = request.arg1;
        command.arg2 = request.arg2;
    }
    else if (strcmp(request.command, "unblock") == 0)
    {
        command.command = MESSAGE;
        command.arg1 = request.arg1;
        command.arg2 = request.arg2;
    }
    else if (strcmp(request.command, "logout") == 0)
    {
        command.command = MESSAGE;
        command.arg1 = request.arg1;
        command.arg2 = request.arg2;
    }
    else
    {

    }
}*/

/* OLD
    //if (*input == 0) { return NULL;}

    Argument *args = Malloc(sizeof(Argument));
    args->string = input;
    args->next = NULL;
    for (char *cur = input; !(cur[0] == 0 && cur[1] == 0); cur++)
    {
        // !! twice checking if cur[0] is 0
        if (*cur == 0)
        {
            args->next = Malloc(sizeof(Argument));
            args->next->string = cur+1;
            args->next->next = NULL;
            args = args->next;
        }
    }
    return args;



// not right?????
char *popArg(Argument **headP)
{
    if (*headP == NULL)
    {
        return NULL;
    }
    Argument *arg = *headP;
    char *string = arg->string;

    *headP = (*headP)->next; // new head = old head -> next
    free(arg); // free removed head
    return string;
}

void freeArgs(Argument *args)
{
    if (args != NULL)
    {
        freeArgs(args->next);
        free(args);
        // SOMETHING IS WRONG WITH ARGS !!
    }
}

// !! LIMIT ON NUMBER OF ARGS?
void getArgs(char *input, char *args[MAXNUMARGS])
{
    args[0] = args[1] = NULL;

    char numArgs = 0; // not really number of args because -1?
    for (char *cur = input; (!(cur[0] == 0 && cur[1] == 0)) && numArgs < MAXNUMARGS; cur++)
    {
        // twice checking if cur[0] == 0. prob would be optimised out easily
        if (*cur == 0)
        {
            args[numArgs] = cur;
            numArgs++;
        }
    }
}


 // maybe not return argument but return through pointer arg thing?
Argument *parseRequest(char *buffer, char **command)
{
    *command = buffer;
    return getArgs(&(buffer[strlen(*command)+1]));
    // args needs to be freed so a bit weird to return it along with something else here?
}


void textToArgs(char *text, struct Request *request)
{
    char **arrs[3] = { &(request->command), &(request->arg1), &(request->arg2) };
    *(arrs[0]) = text;
    char arrsRef = 1;
    for (int i = 1; arrsRef < 3; i++)
    {
        if (text[i] == 0)
        {
            *(arrs[arrsRef]) = &(text[i+1]);
            arrsRef++;
        }
    }
    // assuming null-terminated at end of request->arg2, as protocol specifies
}


void something(char *text)
{
    struct Request request;
    textToArgs(text, &request);
}


char *getArgs(char *text)
{
    return strchr(text, 0) + 1;
}


//enum Command { MESSAGE, BROADCAST, WHOELSE, WHOELSESINCE, BLOCK, UNBLOCK, LOGOUT };
// equivalent to #define 0 blah, #define 1 bloo ?

#include "protocol.h"

struct Command2
{
    enum Command command;
    void *arg1;
    void *arg2;
};

struct Request
{
    // change sizes? Client can enforce them
    char *command;
    char *args;
    //char *arg2;
};


 */
