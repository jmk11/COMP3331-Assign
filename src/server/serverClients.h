#ifndef PROJECT_SERVERCLIENTS_H
#define PROJECT_SERVERCLIENTS_H

#include <stdio.h>
#include "clients.h"

typedef struct SendBuffer SendBuffer;
// sendBuffer definition needs to be here because Connection definition needs to be here and it relies on this
// Maybe I should change Connection to include a pointer instead so this doesn't need to be here !!
struct SendBuffer {
    char sendBuffer[BUFSIZ];
    unsigned int sendBufferSize; // BUFSIZ is 8000 so could be short..
};
typedef struct ConnectionStruct Connection;
/*struct connectionStruct
{
    const int sockfd;
    SendBuffer sendBuffer;
    User *user;

    //char alive; // SHOULD BE MUTEX !! ALSO YUCK?
    //pthread_mutex_t aliveMutex; // is this how it works? I have to have a mutex and the value separately?
};*/
struct ConnectionStruct {
    SendBuffer sendBuffer;
    Client *client;
};

void appendBuffer(SendBuffer *sendBuffer, const char *msg, unsigned int addSize);
ssize_t sendBuffer(Connection *connection);
//ssize_t sendBuffer(Connection *connection, char command);

bool broadcast(User *sender, char *msg, MessageType messageType);

void *handleClient(void *clientSockfdP);

//void debugSendBuffer(Connection *connection);

void setBufferCommand(SendBuffer *sendBuffer, char command);

#endif //PROJECT_SERVERCLIENTS_H
