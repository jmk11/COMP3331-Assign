/*
Defines main() function for server.
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/constants.h"
#include "server.h"
#include "serverPrivate.h"

#include "common/helpers.h"
#include "serverClients.h"

// move to .h?
//User *userList;
//unsigned long blockDuration; // global?
//unsigned long timeout;
//int clientSockfd;
//User *user; // remove???
//char sendData[BUFSIZ];
//int sendDataSize = 0;
//char *welcome;
//ClientsList *clientsList;
// extern:
UserList *userList;
unsigned long timeout;
unsigned long blockDuration; // global?
char *welcome;
ClientsList *clientsList;

//int exitEventfd;

void sigintHandler(int sigNum);

//void *checkMessagesThread(void *connectionP);

// a method to broadcast() to all logged in users. Use this for broadcast, presence notification
// online messaging: ? new thread/s? use broadcast somehow? Use entry from list of connected clients?
// need a way to timeout clients: if each client has only one thread at server, then it can't
// timeout thread?

// TODO do properly with safe
void sigintHandler(int sigNum) {
    // NONE OF THIS IS SAFE!!!!!
    //printf("in Signal handler\n"); // NOT SAFE
    //Close(welcomeSockfd);
    freeClientsList(clientsList);
    freeUserList(userList);
    free(welcome);
    // close all threads and their sockets somehow
    // exit = TRUE or write to eventfd to say exit
    exit(SIGINTEXIT);
}

/*
 * server_port: this is the port number which the server will use to communicate with the
clients. Recall that a TCP socket is NOT uniquely identified by the server port number. So it
is possible for multiple TCP connections to use the same server-side port number.
 * block_duration: this is the duration in seconds for which a user should be blocked after
three unsuccessful authentication attempts.
 * timeout: this is the duration in seconds of inactivity after which a user is logged off by the
server
 */
int main(int argc, char **argv) {
    struct sigaction sa; //= { .sa_handler = intHandler, .sa_mask = 0, .sa_flags = 0 }; // are these assignments right/safe?
    sa.sa_handler = sigintHandler;
    sa.sa_mask = (__sigset_t){{0}};
    sa.sa_flags = 0;
    Sigaction(SIGINT, &sa, NULL);

    unsigned short port;
    //unsigned long blockDuration;
    //unsigned long timeout;
    parseArgs(argc, argv, &port);

    int welcomeSockfd = buildSocket(port);

    parseCredentials();
    if (userList == NULL) {
        printf("Error: No users provided\n");
        return CREDENTIALSERROR;
    }

    welcome = readWelcome();
    clientsList = createClientsList();
    //struct timeval timeouttv;
    //timeouttv.tv_sec = timeout;
    //timeouttv.tv_usec = 0;

    // SINGLE CLIENT:
    int clientSockfd;
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    Client *client;

    while (1) {
        // accept() returns a new socket for this individual connection - the next in the queue
        clientSockfd = Accept(welcomeSockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
        // do poll on accept and eventfd that specifies exit?
        //printf("client connected with ip address: %s\n", inet_ntoa(clientAddr.sin_addr));

        // !!! SET SOCKET RCV TIMEOUT AS SOCK OPTION
        //setsockopt(clientSockfd, SOL_SOCKET, SO_RCVTIMEO, (const void*) &timeouttv, sizeof(timeouttv));
        // this timeout doesn't work with poll, only applies when actually reading data
        // !!!! so if I'm using poll in each client thread, then I need a different timeout mechanism

        // don't really need both clientDetails struct and connection struct - do differently?
        client = pushClient(clientsList, (Client){.user = NULL, .ipaddr = clientAddr.sin_addr.s_addr, .p2pserverport = 0, .sockfd = clientSockfd});
        // I don't think this is a good way. What is the point of the list?
        // I'd need to provide a method to remove used details from the list
        // and this is all becoming very expensive, it must be bad way
        // what about just malloc it and they free it?
        // wait no, I need to keep a user's connection details available to all threads if they need for p2p
        // Could send a request to a particular thread but that seems bad way to do it?
        //cdHeap = Malloc(sizeof(ClientDetails));
        //cdHeap->sockfd = clientSockfd;
        //cdHeap->addr = clientAddr;

        // each client thread needs to know its addr details so it can give them to other users for p2p
        // but, if I just have one clientDetails struct, then it could be overwritten by a new accept call before the client thread can save its details
        // so I have to keep all client addresses permanently? or I could send some kind of message to indicate that one has been copied
        // and can be deleted from list. But that might not be worth it.
        pthread_t clientThread; // !! save this in a list of clients or something?
        Pthread_create(&clientThread, NULL, handleClient, (void *)client);
        // may want to pass struct of clientSockfd & clientAddr
        //handleClient();

        //close(clientSockfd);
    }

    // how to clean exit with all these threads? join?

    Close(welcomeSockfd); // Close?
    freeClientsList(clientsList);
    freeUserList(userList);
    free(welcome);
}

/*
 *
 */
// similarity with client's buildSocket?
int buildSocket(unsigned short port) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    (serverAddr).sin_family = AF_INET;
    // host byte ordering format --> short value in network byte ordering format
    (serverAddr).sin_port = htons(port);
    // host to network long: same as htons but to long
    (serverAddr).sin_addr.s_addr = htonl(INADDR_ANY);

    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    Bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); // binds to port?
    Listen(sockfd, MAXQUEUE);                                         // 5 is max queue length
    return sockfd;
}

// argc int?
void parseArgs(int argc, char **argv, unsigned short *port) {
    if (argc != 4) {
        printf("Usage: %s serverPort blockDuration timeout\n", argv[0]);
        exit(ARGSERROR);
    }
    *port = getPort(argv[1]);
    blockDuration = strtoul(argv[2], NULL, 10); // what happens when error?
    timeout = strtoul(argv[3], NULL, 10);
}

// on global userList
/*
* The valid username and password combinations will be stored in a file called credentials.txt which will be in the same directory as the
server program. Username and passwords are case-sensitive. You may assume that each username and password will be on a separate
        line and that there will be one white space between the two.
*/
// take out userList operations with add() etc.?
void parseCredentials() {
    // FILE or byte stream?
    // for/while ...
    //UserList *head = NULL;
    userList = NULL;
    UserList *cur = NULL;
    char buffer[BUFSIZ] = {0};
    char *usernameEnd, *password;

    FILE *fp = Fopen(USERSFILENAME, "r");
    while (Fgets(buffer, BUFSIZ, fp) != NULL) {
        usernameEnd = strchr(buffer, ' ');
        if (usernameEnd == NULL) {
            printf("Error: %s file incorrectly formatted", USERSFILENAME);
            exit(CREDENTIALSERROR);
        }
        *usernameEnd = 0; // replace space with null byte
        password = usernameEnd + 1;

        if (cur == NULL) {
            cur = Malloc(sizeof(UserList));
            userList = cur;
        } else {
            cur->next = Malloc(sizeof(UserList));
            cur = cur->next;
        }
        cur->user = createUser(buffer, password);
        cur->next = NULL;
    }

    fclose(fp); // error check?
}

/*
void userListAdd(const char *username, const char *password, UserList **cur)
{
    if (*cur == NULL) {
        *cur = Malloc(sizeof(UserList));
        userList = *cur;
    }
    else {
        (*cur)->next = Malloc(sizeof(UserList));
        *cur = (*cur)->next;
    }
    (*cur)->user = createUser(username, password);
    (*cur)->next = NULL;
}*/

char *readWelcome() {
    char *buf = NULL;
    int fd = open(WELCOMEFILE, O_RDONLY);
    if (fd == -1) {
        char *str = "Welcome.\n\n";
        buf = Malloc(strlen(str) + 1);
        strcpy(buf, str);
    } else {
        unsigned long fileSize = Lseek(fd, 0, SEEK_END);
        Lseek(fd, SEEK_SET, 0);
        buf = Malloc(fileSize + 1);
        read(fd, buf, fileSize); // check bytesRead?
        buf[fileSize] = 0;
    }
    close(fd);
    return buf;
}

void freeUserList(UserList *userListR) {
    if (userListR != NULL) {
        freeUserList(userListR->next);
        freeUser(userListR->user);
        free(userListR);
    }
}

// here or in serverClients?
// userList will never be edited after initialisation
// so no lock needed here
// could be improved with some data structure sorted by username?
User *findUser(const UserList *userListR, const char *username) {
    if (userListR != NULL) {
        if (strcmp(getUsername(userListR->user), username) == 0) {
            return userListR->user;
        } else {
            return findUser(userListR->next, username);
        }
    } else {
        return NULL;
    }
}
