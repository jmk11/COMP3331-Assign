/*
Provides main() function for client application
*/

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
//#include <pthread.h>
//#include <error.h>
#include <poll.h>
//#include <assert.h>
#include <unistd.h>
//#include <malloc.h>

//#include "client.h"
#include "common/constants.h"

#include "common/helpers.h"
#include "common/protocol.h"
#include "p2pConnections.h"


// make a linked list of current p2p connections : username, sockfd DONE
//      on startprivate, ADD user to the list  DONE
//      when receive p2p info from server, fill out user's details in list. DONE
//          Must create way to add to pollfds from manageRecv(): return new sockfd through pointer. if not 0, add to pollfds DONE
//      dTODO: p2pfail, remove from list
//      dTODO: on private, match username to retrieve sockfd from list and send message - probably can just send raw message | donetest
//      dTODO: Deal with receiving multiple messages at once
//      dTODO: on stopprivate, close sockfd and REMOVE user from list. Also remove from pollfds.
//              Could keep track of empty spots in pollfds, or maybe just that there is an empty spot (num of empty spots). And person who wants to add will find it (negative)
//      on accept(), add user and details to list DONE
//      dTODO: when other side does stopprivate: remove from list and from pollfds. and print message that session ended.
// add username retrieval to the login process or into p2p response DONE
// add username exchange to p2p handshake DONE

// how to share the user making the private connection?
// should a connector tell the other side what user they are? not secure, also this client doesn't know what user it is
// maybe the server should send a message to the requestee telling them that user has requested a connection from a certain IP
// but no, all the clients have the same IP so I can't differentiate based on that. And can't do IP + port combo because a different port will be used.
// so I guess one client will just have to tell the other their username, after retrieving it from the server.
// I could add username retrieval to the login process, or could add into the p2p response

// findEmptySpot(pollfds)

// THREADS:
// one thread for user input
// one thread for receiving from server - split after login
// one thread for p2p? (receiving?)
// OR !! should I be using select() instead of threads???
//void sendThread(int sockfd); // default thread
//void *receiveThread(void *sockfdP);


ssize_t loginC(int sockfd, char *buffer);
//ssize_t recv2(int sockfd, char *buf, size_t len, int flags);
void printBuf(ssize_t len, char *buf);
void printMessage(char *message);

int buildSocket(struct in_addr IPNetworkOrder, unsigned short portNetworkOrder);

bool manageRecv(char *buffer, ssize_t bytesReceived, char **newPartnerName, int *newSockfd, p2pList **p2plist);
int buildServerSocket(unsigned short *networkOrderPort);

char getCommand(char **message);

bool addToList(p2pNode **p2plist, char *partnerName);

char *getPartnerName(char *argstext, char **restargs);

char *getNextMessage(const char *buffer, ssize_t bytesReceived, char *curMessage);

void handlestdin(int serversockfd, p2pNode **p2plist, char *input, char **removePartnerName);

void removePollfd(unsigned int index, struct pollfd *pollfds, unsigned int *numEmptyPolls);
void addPollfd(unsigned int *pollSize, int newfd, struct pollfd **pollfds, unsigned int *nfds, unsigned int *numEmptyPolls, unsigned int start);

unsigned int getIndex(const struct pollfd *pollfds, int sockfd, unsigned int start, unsigned int nfds);

#define PROMPT ">"

/*
 * server_IP: this is the IP address of the machine on which the server is running.
 * server_port: this is the port number being used by the server. This argument should be the
same as the first argument of the server.
 */
int main (int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: %s serverIP serverPort", argv[0]);
        return ARGSERROR;
    }
    // argv[1] checked when building socket
    unsigned short port = getPort(argv[2]);

    struct in_addr ipaddr;
    Inet_pton(AF_INET, argv[1], &ipaddr);
    int serversockfd = buildSocket(ipaddr, htons(port));
    // htons: host byte ordering format --> short value in network byte ordering format

    char buffer[BUFSIZ]; // make buffer smaller based on agreed size in protocol

    // login:
    ssize_t bytesReceived = loginC(serversockfd, buffer);
    if (bytesReceived > 0) {
        char *curMessage = buffer;
        while (curMessage != NULL) {
            getCommand(&curMessage);
            printMessage(curMessage);
            curMessage = getNextMessage(buffer, bytesReceived, curMessage);
            //printBuf(bytesReceived-1, buffer+1);
        }
    }

    // once logged in successfully, open welcome socket or p2p
    //struct sockaddr_in myAddr;
    //socklen_t myAddrLen = sizeof(myAddr);
    /*GetSockName(sockfd, (struct sockaddr *) &myAddr, &myAddrLen);
    printf("%hu\n", myAddr.sin_port);*/

    unsigned short welcomePort;
    int welcomeSockfd = buildServerSocket(&welcomePort);
    // inform server of port:
    char *portMessage;
    unsigned int length = portToProtocol(welcomePort, &portMessage);
    Send(serversockfd, portMessage, length, 0); // !!! send
    freeProtocolStr(portMessage);

    p2pNode *p2plist = createp2pList();


    // for p2p: could have any number of connections open and needing to be polled
    // though I suppose I could set a hard limit
    // otherwise realloc if the original array size is passed


    // P2P
    // When someone does startprivate <user> , the server sends them the IP and port of its connection with user
    // and the person who sends the startprivate <user> command becomes the CLIENT of <user>'s SERVER
    // So each client needs to run an accept() on the SAME PORT as its connection with the server
    // the accept() could keep the same port or spawn different port
    // so after the connect() call, retrieve the port being used
    // and open a welcoming socket using accept() on this port
    // then each TCP connection opened from that accept() is a person to private message with
    // and spawn off new thread / new poll thing on recv()

    // NO: NOT the same port as its connection with the server
    // Nadeem on course forum:
    // Once your client is listening on a specific port say 5555 (Hint: think about threads here),
    // it connects with the chatting server using a different ephemeral port say 62000
    // and informs the server about the port it is listening on for P2P (5555).
    // The server will provide the client info (IP + 5555) to other peers who wish to start private messaging with this client.

    // this process is as opposed to the person who sends the startprivate <user> becoming the server, and their messaging partner becoming the client
    // which was my original interpretation

    unsigned int numEmptyPolls = 0;
    unsigned int pollSize = 10;
    //struct pollfd pollfds[pollSize];
    struct pollfd *pollfds = Malloc(pollSize*sizeof(struct pollfd));
    unsigned int nfds = 3;
    unsigned char recvPoll = 0;
    unsigned char stdinPoll = 1;
    unsigned char acceptPoll = 2;
    pollfds[recvPoll].fd = serversockfd;
    pollfds[recvPoll].events = POLLIN;
    pollfds[stdinPoll].fd = STDIN_FILENO;
    pollfds[stdinPoll].events = POLLIN;
    pollfds[acceptPoll].fd = welcomeSockfd;
    pollfds[acceptPoll].events = POLLIN;
    unsigned char firstp2p = acceptPoll + 1;

    char *newPartnerName, *removePartnerName;
    unsigned int pollIndex;
    int newsockfd, removeSockfd;
    char input[BUFSIZ];
    ssize_t bytesRead;
    //char *curMessage;
    bool continueSession = TRUE;
    while (continueSession == TRUE) {
        poll(pollfds, nfds, -1);
        if (pollfds[recvPoll].revents & POLLIN) {
            //bytesReceived = recv(sockfd, buffer, BUFSIZ, 0);
            bytesReceived = recv(pollfds[recvPoll].fd, buffer, BUFSIZ, 0); // !! Recv()?
            newPartnerName = NULL;
            newsockfd = 0;
            continueSession = manageRecv(buffer, bytesReceived, &newPartnerName, &newsockfd, &p2plist);
            if (newPartnerName != NULL) {
                setsockfd(p2plist, newPartnerName, newsockfd);
                printf("Start private messaging with %s\n", newPartnerName);
                free(newPartnerName);
                addPollfd(&pollSize, newsockfd, &pollfds, &nfds, &numEmptyPolls, firstp2p);
            }
        }
        else if (pollfds[stdinPoll].revents & POLLIN) {
            //fflush(stdout); // doesn't do what I want
            //putchar('>'); //#define prompt char
            //Fgets(input, BUFSIZ - 1, stdin); // -1 bc protocol - extra 0 at end. Encapsulate with protocol 'allowableSize' function?
            bytesRead = read(pollfds[stdinPoll].fd, &input, BUFSIZ - 1); // !! change to Read?
            input[bytesRead-1] = 0; // remove newline, & null terminate
            removePartnerName = NULL;
            handlestdin(serversockfd, &p2plist, input, &removePartnerName);
            if (removePartnerName != NULL) {
                removeSockfd = getsockfd(p2plist, removePartnerName);
                if (removeSockfd != 0 && removeSockfd != -1) {
                    removeUser(&p2plist, removePartnerName);
                    pollIndex = getIndex(pollfds, removeSockfd, firstp2p, nfds);
                    Close(pollfds[pollIndex].fd);
                    removePollfd(pollIndex, pollfds, &numEmptyPolls);
                }
                else {
                    printf("You do not have a private messaging session with %s.\n", removePartnerName);
                }
                free(removePartnerName);
            }
        }
        else if (pollfds[acceptPoll].revents & POLLIN) {
            int newClientSockfd;
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            newClientSockfd = Accept(welcomeSockfd, (struct sockaddr *) &clientAddr, &clientAddrLen);

            //GetSockName(newClientSockfd, (struct sockaddr *) &myAddr, &myAddrLen);
            //printf("%hu\n", myAddr.sin_port);

            //Send(newClientSockfd,"Hello", 6, 0);
            recv(newClientSockfd, buffer, BUFSIZ, 0);
            printf("%s has started a private messaging session with you.\n", buffer);
            //printf("Received %s\n", buffer);
            // first recv is the username of other person
            addUser(&p2plist, buffer); // what do if connection already exists?
            setsockfd(p2plist, buffer, newClientSockfd);
            addPollfd(&pollSize, newClientSockfd, &pollfds, &nfds, &numEmptyPolls, firstp2p);
        }
        // for i in p2prange:
        //      if (pollfds[i].revents & POLLIN):
        //          bytesReceived = recv(pollfds[i].fd, buffer, BUFSIZ, 0);
        //          if (bytesReceived > 0) { printBuf(bytesReceived, buffer) }
        //          else:
        //              remove connection from list // and move others down the array? seems like waste of time
        //              maybe add this number to a list of empty spots
        //              or set fd to -1 or something to indicate unused (poll() will skip negative fds)
        //              and when looking for a new space, loop through until find fd == -1
        else {
            bool found = FALSE;
            for (unsigned int i = firstp2p; i < nfds && found == FALSE; i++) {
                if (pollfds[i].revents & POLLIN) {
                    found = TRUE;
                    bytesReceived = recv(pollfds[i].fd, buffer, BUFSIZ, 0);
                    char *senderName = findUserName(p2plist, pollfds[i].fd); // go through p2pconnectionslist and find usernamd matching sockfd
                    if (bytesReceived > 0) {
                        char *curMessage = buffer;
                        while (curMessage != NULL) {
                            printf("\t%s(private): %s\n", senderName, curMessage);
                            curMessage = getNextMessage(buffer, bytesReceived, curMessage);
                        }
                    }
                    else {
                        printf("User %s has ended private messaging session\n", senderName);
                        removeUser(&p2plist, senderName);
                        Close(pollfds[i].fd);
                        removePollfd(i, pollfds, &numEmptyPolls);
                    }
                }
            }
            //assert(0);
        }
    }

    printf("Exiting.\n");

    freep2pList(p2plist);
    free(pollfds);
    close(serversockfd);
    close(welcomeSockfd);
    for (unsigned int i = firstp2p; i < nfds; i++) {
        if (pollfds[i].fd != -1) {
            close(pollfds[i].fd);
        }
    }

    // OLD:
    // PROBLEM: !! Server sends "goodbye" message and then closes connection.
    // with this simple loop, client expects more input before it can receive that connection is closed and exit.
    // For now, I'll just add a message in the goodbye to press enter to exit.
    // BUT: Because of shutdown after 3 failed logins, I need to add capacity for server to shutdown client anyway
    // although when client is multithreaded with one thread for receiving and one for user input, this will be dealt
    // with differently anyway
    // Could just do: server sends message of null byte, or two null bytes or something to mean 'exit'.
    // How can one thread end the whole process? Answer: exit()

    // if server sends two messages in a row, then these two messages will be received as one here
    // so second will effectively be ignored
    // so what to do?
    // - end all messages with 00 and check for if more bytes are received after that
    // - always receive fixed number of bytes in one go - that is one message
    // - only send one message at a time - combine longer messages into one. And remove the 'input required / not required' thing
    // - have send/recv combo for every message, even those that don't require input. for those, client sends empty (if possible, or maybe 1 byte)
}

unsigned int getIndex(const struct pollfd *pollfds, int sockfd, unsigned int start, unsigned int nfds)
{
    for (unsigned int i = start; i < nfds; i++) {
        if (pollfds[i].fd == sockfd) {
            return i;
        }
    }
    return 0;
}

// *removePartnerName is filled with malloced memory and needs to be freed by caller
void handlestdin(int serversockfd, p2pNode **p2plist, char *input, char **removePartnerName)
{
    unsigned int length = textToProtocol(input);  // take length from textToProtocol because + 2 to send 2 terminating null bytes
// this is from what the server does
    char *argstext;
    char *command = parseCommand(input, &argstext);
    if (strcmp(command, "startprivate") == 0) {
        char *partnerName = getPartnerName(argstext, NULL);
        if (partnerName == NULL) {
            printf("Invalid arguments.\n");
        }
        else {
            bool success = addToList(p2plist, partnerName);
            if (success == TRUE) {
                send(serversockfd, input, length, 0);
            }
        }
    }
    else if (strcmp(input, "stopprivate") == 0) {
        char *partnerName = getPartnerName(argstext, NULL);
        //bool existed = removeUser(p2plist, partnerName);
        //if (existed == TRUE) {
        if (partnerName != NULL && partnerName[0] != 0) {
            *removePartnerName = Malloc(strlen(partnerName) + 1);
            strcpy(*removePartnerName, partnerName);
        }
        else {
            printf("Invalid arguments.\n");
        }
        //printf("Stop private with user %s requested\n", partnerName);

    }
    else if (strcmp(input, "private") == 0) {
        char *message;
        char *partnerName = getPartnerName(argstext, &message);
        int partnersockfd = getsockfd(*p2plist, partnerName);
        if (partnersockfd == 0) {
            printf("Private messaging with %s has not finished setting up.\n", partnerName);
        }
        else if (partnersockfd == -1) {
            printf("Private messaging with %s is not enabled.\n", partnerName);
        }
        // !!! special error message for if try to message self?
        // but would need to remember username when server gives it
        else {
            send(partnersockfd, message, strlen(message)+1, 0);
            // dTODO: deal with error if partner logs out just before send?
        }
    }
    else {
        send(serversockfd, input, length, 0);
    }
}

void removePollfd(unsigned int index, struct pollfd *pollfds, unsigned int *numEmptyPolls)
{
    pollfds[index].fd = -1; // set pollfd to empty
    *numEmptyPolls += 1;
}

void addPollfd(unsigned int *pollSize, int newfd, struct pollfd **pollfds, unsigned int *nfds, unsigned int *numEmptyPolls, unsigned int start)
{
    unsigned int pollfdpos = *nfds;
    //*nfds += 1;
    bool foundEmpty = FALSE;
    if (*nfds >= *pollSize) { // no more space
        if (*numEmptyPolls > 0) {
            // find empty poll < nfds - fd = -1
            for (unsigned int i = start; foundEmpty == FALSE && i < *nfds; i++) {
                if ((*pollfds)[i].fd == -1) {
                    pollfdpos = i;
                    foundEmpty = TRUE;
                    *numEmptyPolls -= 1;
                }
            }
            if (foundEmpty == FALSE) { // we were lied to. prob should assert so if this does happen I can fix error but..
                *numEmptyPolls = 0;
            }
            //*nfds -= 1; // undo because didn't end up adding new
        }
        if (foundEmpty == FALSE) {
            //*nfds += 1; // nfds is more like lastusedfd
            *pollSize += 10;
            *pollfds = Reallocarray((*pollfds), *pollSize, sizeof(*(*pollfds)));
            // TEST THIS! by reducing original nfds
            //reallocarray checks for overflow in the multiplication
        }
    }
    if (foundEmpty == FALSE) { *nfds += 1; }
    (*pollfds)[pollfdpos].fd = newfd;
    (*pollfds)[pollfdpos].events = POLLIN;
}

// returns first word of args
// and puts pointer to string with rest of args in *restargs, if restargs not null
char *getPartnerName(char *argstext, char **restargs)
{
    char *argsSplit[2];
    parseRequest(argstext, 2, argsSplit); // put first word of args in argsSplit[0]
    if (restargs != NULL) { *restargs = argsSplit[1]; }
    return argsSplit[0];
}

bool addToList(p2pNode **p2plist, char *partnerName)
{
    bool success = FALSE;
    if (partnerName != NULL) { // if parterName is null, server will handle error message
        success = addUser(p2plist, partnerName);
        if (success == FALSE) {
            printf("You already have a private connection with user %s\n", partnerName);
        }
    }
    return success;
}

char getCommand(char **message)
{
    char command = (*message)[0];
    *message = *message+1;
    return command;
}

// !! name
// returns result of last recv() : bytesReceived, buffer
ssize_t loginC(const int sockfd, char *buffer)
{
    char input[BUFSIZ];
    char *message = buffer;
    ssize_t bytesReceived = recv(sockfd, buffer, BUFSIZ, 0);
    // buffer starting with 0 indicates not logged in?
// #define or put in protocol somewhere? !!
    while (bytesReceived > 0 && (getCommand(&message) == LOGIN)) { // maybe the command byte should be after the null?
        // checking last byte assumes that multiple messages won't be received at once. Which is true for the login phase.
        // so I should either put the command byte before the null byte or put it after the null byte with a double null byte signalling final end
        // I'll put it before null byte for now
        //buffer[bytesReceived-2] = 0;
        printf("%s", message);
        //if buffer[0] == INPUTREQUIRED) // input requested
        //printf("%s", PROMPT);
        //fflush(stdout); // this isn't doing what I want
        Fgets(input, BUFSIZ - 1, stdin); // -1 bc protocol - extra 0 at end. Encapsulate with protocol 'allowableSize' function?
        //input[strlen(input)-1] = 0;
        //if (strcmp(input, "quit") == 0) { break; }
        unsigned int length = textToProtocol(input);
        Send(sockfd, input, length, 0); // + 2 to send 2 terminating null bytes
        bytesReceived = recv(sockfd, buffer, BUFSIZ, 0);
        message = buffer;
    }
    return bytesReceived;
}

/*
 *
 */
int buildSocket(struct in_addr IPNetworkOrder, unsigned short portNetworkOrder)
{
    struct sockaddr_in serverAddr; // needs to be declared in main?
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = portNetworkOrder;
    memcpy(&serverAddr.sin_addr.s_addr, &IPNetworkOrder, sizeof(IPNetworkOrder));
    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    Connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    // should Connect be here?
    return sockfd;
}

// sets *newPartnername to malloced memory. caller needs to free
bool manageRecv(char *buffer, ssize_t bytesReceived, char **newPartnerName, int *newSockfd, p2pList **p2plist)
{
    bool continueSession = TRUE;
    //*newParterName = NULL;
    //*newSockfd = 0;

    if (bytesReceived > 0) {
        char *curMessage = buffer;
        char command;
        while (curMessage != NULL) {
            command = getCommand(&curMessage);
            if (command == PRINT) {
                printMessage(curMessage);
            }
            else if (command == LOGIN) {
                //assert(0);
            }
            else if (command == P2P) {
                uint32_t ipaddr;
                unsigned short port;
                char *partnerName;
                char *myName;
                curMessage = protocolToAddr(&ipaddr, &port, &partnerName, &myName, curMessage); // return: moves curMessage along to end of message
                //printf("IP: %u\tPort: %hu\tUser: %s\n", ipaddr, port, partnerName);
                // the ipaddr and port should be all messed up bc network byte order
                // add to list - if user already in list
                // otherwise - this connection wasn't requested
                int sockfd = buildSocket((struct in_addr) {.s_addr = ipaddr }, port);
                // save newsockfd in list attached to username
                // add to pollfds somehow - return newsockfd
                *newPartnerName = Malloc(strlen(partnerName)+1);
                strcpy(*newPartnerName, partnerName);
                *newSockfd = sockfd;

                //char str[MAXLENGTH];
                //snprintf(str, MAXLENGTH, "Hello server %s", parterName);
                Send(sockfd, myName, strlen(myName)+1, 0);
                //printf("Sent %s\n", myName);
            }
            else if (command == P2PFAIL) {
                char *partnerName = curMessage;
                removeUser(p2plist, partnerName); // how get the partner name?
                // sockets and stuff haven't been made yet
                // I could change it so that someone is added to the list only when the server sends a successful response
                // but I didn't want to do that because it allows the server to make client start p2p with anyone even if client didn't ask
                // else I need to make a new protocol thing for p2pfail: username of failed attempt is given
                // and then probably send the 'failed' message as separate print message
            }
            else {
                //assert(0);
            }
            curMessage = getNextMessage(buffer, bytesReceived, curMessage);
            //printBuf(bytesReceived, buffer);
        }
    }
    else { continueSession = FALSE; }
    return continueSession;
}

char *getNextMessage(const char *buffer, ssize_t bytesReceived, char *curMessage)
{
    while ((curMessage < buffer + bytesReceived) && curMessage[0] != 0) {
        curMessage++;
    }
    curMessage++; // point to start of next message
    if (curMessage >= buffer + bytesReceived) { curMessage = NULL; }
    return curMessage;
}

/* len includes terminating null byte
 * assumes len > 0
 * You never know how many messages you'll get in one go.
 * So need to find all the separate messages and print them.
 * Every null byte before the final null byte (located at len-1) indicates the end of a message that is not the last message
 * I didn't think to include this for the first buffer after logging until I tested on CSE, and the offline messages didn't appear
 * I guess CSE managed the switching between processes differently to my computer
 */
void printBuf(ssize_t len, char *buf)
{
    printf("%s", buf);
    for (size_t i = 0; i < len - 1; i++) {
        if (buf[i] == 0) {
            printf("%s", &(buf[i+1]));
        }
    }
}

// why does this function exist?
void printMessage(char *message)
{
    printf("%s", message);
}

// port must be in network byte order already
int buildServerSocket(unsigned short *networkOrderPort)
{
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    //serverAddr.sin_port = networkOrderPort;
    serverAddr.sin_port = 0; // apparently this means random port is selected
    // host to network long: same as htons but to long
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)); // binds to port?
    Listen(sockfd, MAXQUEUE); // 5 is max queue length

    struct sockaddr_in myAddr;
    socklen_t myAddrLen = sizeof(myAddr);
    GetSockName(sockfd, (struct sockaddr *) &myAddr, &myAddrLen);
    //printf("%hu\n", myAddr.sin_port);
    *networkOrderPort = myAddr.sin_port;

    return sockfd;
}


/*
    if (bytesReceived > 0) { // did loginC end successfully?
        printf("%s", buffer);
        // create receiving thread
        pthread_t recvThread;
        //Pthread_create(&recvThread, NULL, receiveThread, &sockfd);
        Pthread_create(&recvThread, NULL, receiveThread, (void *) sockfd); // !!?? what if I just pass the value, cast as a void pointer?
        // the recvThread function knows what its getting...
        // passing shared memory sockfd? seems bad? I know nothing will change it but still
        // NULL attributes ???

        sendThread(sockfd);
        Pthread_join(recvThread, NULL); // will never really happen because intended exit of program is recvThread calling exit()
    }*/

/*
int buildSocket(char *IP, unsigned short port)
{
    struct sockaddr_in serverAddr; // needs to be declared in main?
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    // host byte ordering format --> short value in network byte ordering format
    serverAddr.sin_port = htons(port);
    int success = inet_pton(AF_INET, IP, &(serverAddr.sin_addr));
    if (success == 0) {
        printf("Error: Provided HostIP is invalid\n");
        exit(1);
    }

    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    Connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    // should Connect be here?
    return sockfd;
}*/

// name - input?
/*
void sendThread(int sockfd)
{
    char input[BUFSIZ];
    while (1) {
        //printf("%s", PROMPT);
        fflush(stdout); // doesn't do what I want
        //putchar('>'); //#define prompt char
        Fgets(input, BUFSIZ - 1, stdin); // -1 bc protocol - extra 0 at end. Encapsulate with protocol 'allowableSize' function?
        //input[strlen(input)-1] = 0;
        if (strcmp(input, "quit") == 0) { exit(0); } // !! remove?
        unsigned int length = textToProtocol(input);  // take length from textToProtocol because + 2 to send 2 terminating null bytes
        send(sockfd, input, length, 0);
    }
}*/

// name
/*
void *receiveThread(void *sockfdP)
{
    //int sockfd = * ((int *) (sockfdP));
    int sockfd = (int) sockfdP;

    char buffer[BUFSIZ];
    ssize_t bytesReceived = recv2(sockfd, buffer, BUFSIZ, 0);
    while (bytesReceived > 0)
    {
        printf("%s", buffer);
        //if buffer[0] == INPUTREQUIRED) // input requested
        bytesReceived = recv(sockfd, buffer, BUFSIZ, 0);
    }
    printf("Exiting.\n");
    exit(0);
}*/

/*
ssize_t recv2(int sockfd, char *buf, size_t len, int flags)
{
    ssize_t bytesReceived = recv(sockfd, buf, len, flags);
    for (size_t i = 0; (bytesReceived > 0) && (i < bytesReceived - 1); i++) {
        if (buf[i] == 0) { buf[i] = ' '; }
        // maybe server could not newline messages, and client replaces null byte with newline?
        // could be a problem with trailing spaces when testing?
        // could also do by calling print on the byte after each null byte
    }
    // remove intermediate nulls in case receives two messages at once
    // should put in protocol.c or something?
    return bytesReceived;
}*/
