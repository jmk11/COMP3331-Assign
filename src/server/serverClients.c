/*
Manages an individual client connection.
*/

#include <poll.h>
#include <stdbool.h>

#include "server.h"

#include "common/helpers.h"
#include "serverClients.h"
#include "commands.h"
#include "common/protocol.h" // #included somewhere else is that why I commmented this?

User *execLogin(Connection *connection);
void getDetails(Connection *connection, char username[MAXLENGTH], char password[MAXLENGTH]);
bool doLogin(const char username[MAXLENGTH], const char password[MAXLENGTH], char msg[MAXLENGTH], User **user);
//void checkMessages(User *user, SendBuffer *sendBuffer);
//bool communicate(Connection *connection, char buffer[BUFSIZ]);
void broadcastLogIn(Connection *connection);
//void joinCheckMessages(Connection *connection, pthread_t checkMsgsThread);
void bufferMessage(SendBuffer *sendBuffer, const char *sender, const char *msg, MessageType messageType);
void processLogout(Connection *connection, bool connectionAlive);
void checkMessages2(Connection *connection);
//void checkOfflineMessages(User *user, SendBuffer *sendBuffer, int eventfd);
bool manageRecv(char *buffer, Connection *connection, bool *connectionAlive);
//void setBufferNotLoggedIn(SendBuffer *sendBuffer);
char *getNextMessage(const char *buffer, ssize_t bytesReceived, char *curMessage);

void initialiseBufferCommand(SendBuffer *sendBuffer);

/*
 *
*/
void *handleClient(void *client) {
    // copy client data over to connection struct?
    // or make different connection struct that just includes pointer to client?
    /*Connection connection = {.sockfd = getSockfd(client),
                             .sendBuffer = { .sendBuffer = {0}, .sendBufferSize = 0 },
                             .user = NULL,
                             //.alive = 1,
                             //.aliveMutex = PTHREAD_MUTEX_INITIALIZER // MUTEX! OR SOMETHING ELSE
                            }; // use INITIALIZER or function?*/
    //Pthread_mutex_init(&connection.aliveMutex, NULL);
    Connection connection = {.sendBuffer = {.sendBuffer = {0}, .sendBufferSize = 0},
                             .client = client};
    initialiseBufferCommand(&connection.sendBuffer);

    appendBuffer(&connection.sendBuffer, welcome, 0);

    /// login
    //connection.user = NULL;
    //connection.user = execLogin(&connection);
    connection.client->user = NULL;
    connection.client->user = execLogin(&connection);

    if (connection.client->user == NULL) {
        sendBuffer(&connection);
    } else { //(connection.user != NULL) { // might be clearer to exit if user = null?
        //updateTimeActive(connection.client->user); // !! IS THIS HOW WHOELSESINCE IS SUPPOSED TO WORK?? update when user logs in or when user is active?
        // no - whoelsesince is supposed to give everyone online plus people who have been online in the last x seconds
        // ie timeactive should be updated on logout
        //checkMessages(connection.user, &connection.sendBuffer); // remove this somehow?
        /*if (hasMessages(connection.user)) { // !! REMOVE DOUBLE CHECK
            appendBuffer(&connection.sendBuffer, "You received messages while offline:\n");
            checkMessages(connection.user, &connection.sendBuffer);
        }*/

        appendBuffer(&connection.sendBuffer, "\n", 0);
        //appendBuffer(PROMPT);
        sendBuffer(&connection);

        broadcastLogIn(&connection);

        // select()
        // with checkMessages - use a pipe? store both the read and write fds in the MessageList struct. write a byte to the
        // or use eventfd:
        unsigned short recvPoll = 0;
        unsigned short msgsPoll = 1;
        //unsigned short requestsPoll = 2;
        unsigned short recvTimerPoll = 2;
        unsigned char nfds = 3;
        struct pollfd pollfds[nfds];
        pollfds[recvPoll].fd = connection.client->sockfd;
        pollfds[recvPoll].events = POLLIN; // for reading
        //pollfds[msgsPoll].fd = checkMsgsEventfd;
        pollfds[msgsPoll].fd = getMsgsEventfd(connection.client->user);
        pollfds[msgsPoll].events = POLLIN;
        //pollfds[requestsPoll].fd = getRequestsEventfd(connection.client->user);
        //pollfds[requestsPoll].events = POLLIN;
        pollfds[recvTimerPoll].fd = Timerfd_create(CLKID, 0);
        pollfds[recvTimerPoll].events = POLLIN;
        // should I implement the msgsPoll as one event implies one message? yes I think so, make it semaphore

        struct itimerspec its = {.it_value.tv_sec = timeout, .it_value.tv_nsec = 0, .it_interval.tv_sec = 0, .it_interval.tv_nsec = 0};
        // CHANGE TIMEOUT TO USE NANOSECONDS INSTEAD OF SECONDS?
        Timerfd_settime(pollfds[recvTimerPoll].fd, 0, &its, NULL); // perhaps I should be starting the timer earlier?

        //uint64_t eventfdCount;// better than long because specifies 64 bits?
        //checkOfflineMessages(connection.user, &connection.sendBuffer, pollfds[msgsPoll].fd);

        char buffer[BUFSIZ];
        //ssize_t bytesReceived = -1;
        bool connectionAlive = TRUE;
        bool continueSession = TRUE;
        while (continueSession == TRUE) {
            // how to do timeout? Can make poll timeout at timeout, then do a recv and should say that connection is closed
            // but if the user receives messages every 5 seconds, then it will never timeout even though they aren't typing anything
            // I need the thread to wake up when it has been t seconds since the last recv
            // or maybe there is some kind of timeout fd I can poll? Or I would need a separate thread to set it
            // TIMERFD!
            // I could have another thread that receives a message that a recv has happened, starts timer
            // and notifies this when timer expired
            // but the timer thread would need to be asleep and wake up when either a) recv happened or b) timeout expired
            // so still has the same problem...
            // I could implement timeout with the socket option if I had multiple separate threads for each client: one for recv one for msgs, one for requests
            // I don't want to do that
            poll(pollfds, nfds, -1);                  // infinite timeout // (timeout+1)*1000
            if (pollfds[recvPoll].revents & POLLIN) { // revents is a bit string
                // recv is ready
                continueSession = manageRecv(buffer, &connection, &connectionAlive);
                // !! what if receive multiple messages at once? handled in manageRecv()
                Timerfd_settime(pollfds[recvTimerPoll].fd, 0, &its, NULL); // reset timer
            } else if (pollfds[msgsPoll].revents & POLLIN) {
                // just print offline messages same as others. for now.
                // read the (as in one) message
                //read(pollfds[msgsPoll].fd, &eventfdCount, sizeof eventfdCount); // move read to messages, inside mutex? !!
                //printf("here %s\n", getUsername(connection.client->user));
                // what do do with the count? it doesn't really matter I just want to decrement it
                // since I know it is not 0
                checkMessages2(&connection); // deals with decrementing eventfd
            } else if (pollfds[recvTimerPoll].revents & POLLIN) {
                // reading returns number of times expired, don't need to do it?
                appendBuffer(&connection.sendBuffer, "\nYou have timed out.", 0);
                continueSession = FALSE;
            } else {
                //assert(0);
            }
        }
        //sendBuffer(&connection); // !! will fail if bytesReceived = 0 was reason for exit? // why is this here anyway?

        // logout if client exited without logging out?
        processLogout(&connection, connectionAlive); // it says bytesReceived may be unitialised but I am sure this is false
        Close(pollfds[recvTimerPoll].fd);
        // does x >= 0 always return 1 or 0?
        // select() vs poll(): poll() doesn't require remaking events before every call
        // vs epoll():
    }
    Close(connection.client->sockfd); // send FIN
    // remove node from clientsList by providing client pointer ... weird ...
    removeClientNode(clientsList, client);
    return NULL; // return something? idk
}

bool manageRecv(char *buffer, Connection *connection, bool *connectionAlive) {
    bool continueSession = TRUE;
    char *args;
    *connectionAlive = TRUE;

    ssize_t bytesReceived = recv(connection->client->sockfd, buffer, BUFSIZ, 0);
    if (bytesReceived <= 0) {
        continueSession = FALSE;
        *connectionAlive = FALSE;
    } else {
        buffer[MIN(bytesReceived, BUFSIZ - 1)] = 0; // !! remove? or adjust to add 2 null bytes in case client didn't?
        char *curMessage = buffer;
        while (curMessage != NULL) {
            //printf("received:'%s'\n", buffer);
            //continueSession = communicate(connection, buffer);
            char *command = parseCommand(curMessage, &args);
            bool (*execCommand)(char *, Connection *) = matchCommand(command); // just for fun
            continueSession = execCommand(args, connection);
            sendBuffer(connection); // send buffer even if not continue session?
            // updateTimeActive(connection->client->user);
            // IT SEEMS that whoelsesince should take every who is online, plus people who were logged in
            // so NOT based on commands entered
            // dTODO: add capacity for server to split messages received multiple at once? !
            // by splitting at double null byte, very similar to how client does it
            curMessage = getNextMessage(buffer, bytesReceived, curMessage);
        }
    }
    return continueSession;
}

void processLogout(Connection *connection, bool connectionAlive) {
    if (connectionAlive == TRUE) {
        appendBuffer(&connection->sendBuffer, "\nLogging out. Goodbye.\n", 0);
        sendBuffer(connection);
    }
    logout(connection->client->user);
    // more?

    // notify others of logout - broadcast
    char str[MAXLENGTH]; // name !!
    snprintf(str, MAXLENGTH, "%s HAS LOGGED OUT", getUsername(connection->client->user));
    broadcast(connection->client->user, str, SERVER_MESSAGE);
}

void broadcastLogIn(Connection *connection) {
    char str[MAXLENGTH]; // name !!
    snprintf(str, MAXLENGTH, "%s HAS LOGGED IN", getUsername(connection->client->user));
    broadcast(connection->client->user, str, SERVER_MESSAGE);
}

// the head of messageList could only have changed between retrieveMessage and clearMessage() if another thread was
// running this same function on the same user.
// should be impossible because of other checks
// get one message only
void checkMessages2(Connection *connection) {
    char *sender;
    char *msg;
    MessageType messageType;
    //char str[MAXLENGTH] = {0};
    //int sendResult = 0;
    //printf("cm2 %s\n", getUsername(connection->client->user));

    retrieveMessage(connection->client->user, &sender, &msg, &messageType); // double semaphore
    //printf("cm2 %s - %s\n", getUsername(connection->client->user), msg);
    bufferMessage(&connection->sendBuffer, sender, msg, messageType);
    clearMessage(connection->client->user); // decrements eventfd
    //debugSendBuffer(connection);
    sendBuffer(connection);
    //debugSendBuffer(connection);
}

void bufferMessage(SendBuffer *sendBuffer, const char *sender, const char *msg, MessageType messageType) {
    char str[MAXLENGTH];
    if (messageType == USER_MESSAGE) {
        snprintf(str, MAXLENGTH, "\t%s: %s\n", sender, msg);
        // \t to make different from input, since prompt isn't working. Other ideas?
        appendBuffer(sendBuffer, str, 0);
    } else if (messageType == SERVER_MESSAGE) {
        snprintf(str, MAXLENGTH, "\tSERVER: %s\n", msg);
        appendBuffer(sendBuffer, str, 0);
    }
    //else { assert (0); }
}

User *execLogin(Connection *connection) {
    //assert(connection->sendBuffer.sendBufferSize == 0); // assert buffer is empty
    //setBufferNotLoggedIn();

    //send(clientSockfd, WELCOME, strlen(WELCOME)+1, 0);

    char username[MAXLENGTH] = {0}; // LENGTHS. DEFINE IN PROTOCOL?
    char password[MAXLENGTH] = {0};
    char message[MAXLENGTH] = {0};
    User *user = NULL;
    bool continueLogin = TRUE;
    while (continueLogin == TRUE) { //&& user == NULL) { // If I'm gunna be checking then continueLogin then no point checking user as well
        getDetails(connection, username, password);
        continueLogin = doLogin(username, password, message, &user);
        appendBuffer(&connection->sendBuffer, message, 0);
        // continueLogin will be false if login successful or 3 unsuccessful attempts
    }
    return user;
}

// need maxlength here?
// msg: return message in it
// char return: whether to continue or exit from connection. !! I don't like it.
// Only one thing returns differently and it's hidden. Plus I'm not sure this function should make that kind of decision.
bool doLogin(const char username[MAXLENGTH], const char password[MAXLENGTH], char msg[MAXLENGTH], User **user) {
    *user = NULL;
    //char str[MAXLENGTH] = {0};

    User *attemptUser = findUser(userList, username);          // not sure about having this here ..
    char result = login(attemptUser, password, blockDuration); // validate that user was found here or in login()?
    //NEW?: User *loginUser = findUser(userList, username);
    // but user.c would have to know structure of userListalt. #include server.h? I don't like that.
    // define userListalt in user.h? But I wanted the server to own it.
    // I would need to: define findUser() in server, call getUsername on every user 'object' to do comparison
    //NEW?: char result = login(loginUser, password, blockDuration);
    if (result == LOGINSUCCESSFUL) {
        //str[0] = INPUTREQUIRED;
        *user = attemptUser;
        snprintf(msg, MAXLENGTH, "Login Successful!\nWelcome %s.\n\n", username);
        return FALSE;
        //setBufferLoggedIn(); // not sure about putting this here? Maybe in handleClient()?
        //strlcat2(sendData, str, BUFSIZ, &sendDataSize);
        //NEW?: user = loginUser;
    } else if (result == INVALIDUSERNAME) {
        //str[0] = NOINPUT;
        snprintf(msg, MAXLENGTH, "Login Failed.\nUser %s does not exist.\n\n", username);
    } else if (result == LOGGEDIN) {
        snprintf(msg, MAXLENGTH, "Login Failed.\nUser %s is already logged in.\n\n", username);
    } else if (result == INCORRECTPASSWORD) {
        //str[0] = NOINPUT;
        snprintf(msg, MAXLENGTH, "Login Failed.\nPassword incorrect.\n\n");
    } else if (result == NOWBARRED) {
        // !!!! Spec : "The terminal should shut down at this point." !!!
        // Server needs to send message to client telling them to shut down
        snprintf(msg, MAXLENGTH,
                 "Login Failed.\nPassword incorrect.\nAccount %s has been blocked due to %d incorrect attempts. Please try again later.\n\n",
                 username, MAXATTEMPTS);
        return FALSE;
    } else if (result == BARRED) {
        //str[0] = NOINPUT;
        snprintf(msg, MAXLENGTH,
                 "Login Failed.\nUser %s is barred due to %d invalid attempts. Please try again later.\n\n",
                 username, MAXATTEMPTS);
        // strings in #define or something like that ??
    } else {
        //assert(0);
    }

    //send(clientSockfd, str, strlen(str+1)+2, 0);
    //appendBuffer(msg);
    // what if just returned a string allocated in this function?
    // if it is used straight away the memory won't be overriden yet? ...
    // cuz would be nice if just return string instead of having to take sendBuffer details
    // could use malloced string
    //return user;

    return TRUE;
}

// need maxlength here?
void getDetails(Connection *connection, char username[MAXLENGTH], char password[MAXLENGTH]) {
    char str[MAXLENGTH] = {0};
    //str[0] = INPUTREQUIRED;
    //snprintf(str, MAXLENGTH, "Please log in.\n%sUsername: ", PROMPT);
    snprintf(str, MAXLENGTH, "Please log in.\nUsername: ");
    appendBuffer(&connection->sendBuffer, str, 0);
    //send(clientSockfd, str, strlen(str + 1) + 2, 0);
    //setBufferNotLoggedIn(&connection->sendBuffer); // needs to be done every time before sending
    setBufferCommand(&connection->sendBuffer, LOGIN);
    sendBuffer(connection);
    //setBufferNotLoggedIn();
    //ssize_t bytesReceived = recv(connection->client->sockfd, username, MAXLENGTH, 0); // null byte transmitted
    recv(connection->client->sockfd, username, MAXLENGTH, 0); // null byte transmitted
    // should I assume the null byte is transmitted by client? Prob no.

    //str[0] = INPUTREQUIRED;
    //snprintf(str, MAXLENGTH, "%sPassword: ", PROMPT);
    snprintf(str, MAXLENGTH, "Password: ");
    appendBuffer(&connection->sendBuffer, str, 0);
    //setBufferNotLoggedIn(&connection->sendBuffer);
    setBufferCommand(&connection->sendBuffer, LOGIN);
    sendBuffer(connection);
    //setBufferNotLoggedIn(); // needs to be done every time after sending
    //send(clientSockfd, str, strlen(str + 1) + 2, 0);
    //bytesReceived = recv(connection->client->sockfd, password, MAXLENGTH, 0);
    recv(connection->client->sockfd, password, MAXLENGTH, 0);
    // !!! need to deal with case where client exits while this is waiting for their username/password

    // client sending their P2P port: should add another stage to login process,
    // or change protocol so client can send message with command to server?
    // I can just keep the command text - add another text command 'setupp2p' or 'provideport' that isn't actually typed by user
    // And server will then parse it - differently to other companies because contains binary info
    // for a real service I think it would be better if providing a p2p port wasn't a required part of the login process, so clients could opt out
    // hmm, protocol is very inconsistent. client->server is text, server->client has plenty not text
}

void initialiseBufferCommand(SendBuffer *sendBuffer) {
    sendBuffer->sendBuffer[0] = PRINT;
}

void appendBuffer(SendBuffer *sendBuffer, const char *msg, unsigned int addSize) {
    //strlcat2(sendBuffer->sendBuffer, msg, BUFSIZ, &sendBuffer->sendBufferSize);
    strlcat2(&sendBuffer->sendBuffer[1], msg, BUFSIZ - 1, &sendBuffer->sendBufferSize, addSize);
}

ssize_t sendBuffer(Connection *connection) {
    ssize_t result = 0; // a bit dishonest returning this when maybe the send() would have returned error. This is not a problem now I'm using Send()?
    if (connection->sendBuffer.sendBufferSize != 0) {
        // unintended behaviour of strlcat2: If I give it an empty string, it will append a null byte, and won't increment the buffer size
        // that works out, so I'll use it to add the second null byte
        //appendBuffer(&connection->sendBuffer, ""); // check this in gdb !!
        result = Send(connection->client->sockfd, connection->sendBuffer.sendBuffer, connection->sendBuffer.sendBufferSize + 2, 0);
        // +2 because buffer size does not include command byte
        connection->sendBuffer.sendBufferSize = 0;
        connection->sendBuffer.sendBuffer[0] = PRINT; // command byte. print is default
        connection->sendBuffer.sendBuffer[1] = 0;     // start of message
    }
    return result;
}

void setBufferCommand(SendBuffer *sendBuffer, char command) {
    sendBuffer->sendBuffer[0] = command;
}

char *getNextMessage(const char *buffer, ssize_t bytesReceived, char *curMessage) {
    while ((curMessage < buffer + bytesReceived) && !(curMessage[0] == 0 && curMessage[1] == 0)) {
        curMessage++;
    }
    //curMessage[0] is the first of the two zeroes
    curMessage += 2; // point to start of next message
    if (curMessage >= buffer + bytesReceived) {
        curMessage = NULL;
    }
    return curMessage;
}

// returns whether your broadcast went to everyone - 1 yes 0 no
// "blocked users also do not get presence notifications"
// blockee: doesn't get presence notifications, does get messages
// blocker: does get presence notifications   , doesn't get messages (as in spec + forums)
bool broadcast(User *sender, char *msg, MessageType messageType) {
    bool complete = true;
    for (UserList *cur = userList; cur != NULL; cur = cur->next) {
        if (cur->user != sender && (isLoggedIn(cur->user) == TRUE)) {
            if (!(messageType == SERVER_MESSAGE && (isBlocked(sender, cur->user) == TRUE)) && // presence notifications: sender has not blocked receiver
                !(messageType == USER_MESSAGE && (isBlocked(cur->user, sender) == TRUE))) {   // user messages: receiver has not blocked sender
                addMessage(sender, cur->user, msg, messageType);
            } else {
                complete = false;
            }
        }
    }
    return complete;
}

/*else if (pollfds[requestsPoll].revents & POLLIN) {
                // just decrement it and do nothing for now
                // what do do if logout while request is pending ?
                // !!! keep for next time or discard?
                char *requesterName;
                IP ip;
                unsigned short port;
                retrieveRequest(connection.client->user, &requesterName, &ip, &port);
                char str[MAXLENGTH];
                snprintf(str, MAXLENGTH, "P2P request from %s for %s connecting to IP ? and port %hu\n", requesterName, getUsername(connection.client->user), port);
                appendBuffer(&connection.sendBuffer, str);
                sendBuffer(&connection);
                // send IP and port to this user
                // client program then connects and deals with that
                // possibly private messages sent by client send message to server to say message is sent to stop timeout
                // but lots of load at server for not much
                // maybe change client so, if private connections open, doesn't exit at timeout?
                // but that might not be allowed
                // and add connection to server list of active p2p connections? so don't allow double up?
                clearRequest(connection.client->user);
            }*/

//from inside handleclient:
/*
        char continueSession = 1;
        char buffer[BUFSIZ];
        int bytesReceived = recv(connection.sockfd, buffer, BUFSIZ, 0);
        while (bytesReceived > 0 && continueSession) {
            communicate(&connection, &bytesReceived, &continueSession, buffer);
        }
        appendBuffer(&connection.sendBuffer, "Goodbye.\n\n"); // !! THIS NEVER GETS SENT // now it does - processLogout
        //sendBuffer(&connection); // !! will fail if bytesReceived = 0 was reason for exit? // why is this here anyway?

        joinCheckMessages(&connection, checkMsgsThread);

        // logout if client exited without logging out?
        processLogout(&connection);
        */

/*
void debugSendBuffer(Connection *connection)
{
    printf("sendBufferSize = %d\n", connection->sendBuffer.sendBufferSize);
    printf("sendBuffer: %s\n", connection->sendBuffer.sendBuffer);
}
 */

/*
void test()
{
    while(1) {
        char *str = "This is a test\n";
        send(clientSockfd, str, strlen(str) + 1, 0);
        sleep(5);
    }
}*/

/*
bool communicate(Connection *connection, char buffer[BUFSIZ])
{
    bool continueSession;
    char *args;

    char *command = parseCommand(buffer, &args);
    //parseRequest(buffer, words); // returns pointers to different spots in buffer
    char (*execCommand)(char *, Connection *) = matchCommand(command); // just for fun
// Duh! function pointers aren't weird, they look that way because it is the same as declaring a function
    continueSession = execCommand(args, connection);
    sendBuffer(connection); // send buffer even if not continue session?
    updateTimeActive(connection->client->user);
    return continueSession;
}*/

/*
// return something instead of the two pointers?
void communicate(Connection *connection, int *bytesReceived, char *continueSession, char buffer[BUFSIZ])
{
    //char buffer[BUFSIZ];
    buffer[MIN((*bytesReceived), BUFSIZ - 1)] = 0; // !! remove? or adjust to add 2 null bytes in case client didn't?
    printf("received:'%s'\n", buffer);

    char *args;
    char *command = parseCommand(buffer, &args);
    //parseRequest(buffer, words); // returns pointers to different spots in buffer

    char (*execCommand)(char *, Connection *) = matchCommand(command); // just for fun
// Duh! function pointers aren't weird, they look that way because it is the same as declaring a function
    *continueSession = execCommand(args, connection);
    sendBuffer(connection); // send buffer even if not continue session?
    if (*continueSession) {
        // move append \n to here?
        //appendBuffer(PROMPT);

        updateTimeActive(connection->user);

        //sendBuffer(&connection); // send buffer even if not continue session?
        *bytesReceived = recv(connection->sockfd, buffer, BUFSIZ, 0);
        // timeout: Can't timeout because is sitting around waiting for recv!
        // need a separate thread to do timeouts?
        // and that thread needs to stop this one somehow
        // timedout if lastSeen + timeoutDuration < time // <=?
    }
}*/

/*
// the head of messageList could only have changed between retrieveMessage and clearMessage() if another thread was
// running this same function on the same user.
// should be impossible because of other checks
void *checkMessagesThread(void *connectionP)
{
    // will continue after connection is closed - it doesn't know
    // if the sockfd is reassigned, it will run the checks on the same user and send to the sockfd
    // which now belongs to a different user

    Connection *connection = (Connection *) connectionP;
    char *sender;
    char *msg;
    MessageType messageType;
    //char str[MAXLENGTH] = {0};
    //int sendResult = 0;

    // NOT SAFE ALSO BAD !!!!
    // eg connection could be killed while this is halfway through the while loop, still executes send etc.
    // WAIT this won't even work a bit. Because retrieveMessage will just block forever
   //while (connection->alive) { // WHILE WHAT ?? !! // until send error?
   //while (sendResult >= 0) { // bad. incl. assuming there is some gap between previous connection ending and new starting
        // Messages semaphore from connection->user - messages available not 0
        // inside:
        //checkMessages(connection->user, &connection->sendBuffer); // will change once semaphore is here - remove if hasMessages()
        //sendBuffer(connection); // PROBLEM: could send an incomplete buffer

        // this nested loop is pointless. Or is it?
        // should I implement the semaphore here or just in retrieveMessage? seems harder here.

        /// what about: block on hasMessages(), when it finds a message, set semaphore, mutex
        /// then retrieveMessage, clearMessage, unlock


    char alive = connection->alive;
    while (alive && retrieveMessage(connection->user, &sender, &msg, &messageType)) { // retrieveMessage() will block
        // STILL PROBLEM: thread will exist until someone sends a message !!
        // then what? message will have been retrieved, and semaphore decremented while message hasn't been accessed
        // which means message will be lost, and also means that hasMessages() will be true while semaphore will be 0
        // so login will wait for message. and mismatch between hasMesages and semaphore will persist and prevent everything
        // for now, I am going to recrement semaphore at end of getMessage() and wait again in clearMessage()
        // this means that, atm, this message isn't lost. But still need someone to message before logout can finish
        // !! need to cancel wait? sig handler?
        // or wait on multiple objects - message available / connection shutdown
        // what is the point of this mutex?
        Pthread_mutex_lock(&connection->aliveMutex);
        if (connection->alive) {
            bufferMessage(&connection->sendBuffer, sender, msg, messageType);
            clearMessage(connection->user);
            sendBuffer(connection); // !! atm, sending messages one at a time because of the retrieveMessage() blocking thing
            // I need the block, but I want to know before it will block, so I can send the buffer
            // or maybe this way is better anyway
        }
        alive = connection->alive; // I want to lock in the decision to exit while in the lock
        Pthread_mutex_unlock(&connection->aliveMutex);
        // what's with the alive & connection->alive? Surely I don't need both?
    }

    return NULL;
}*/

/*
void joinCheckMessages(Connection *connection, pthread_t checkMsgsThread)
{
    Pthread_mutex_lock(&connection->aliveMutex);
    connection->alive = 0;
    Pthread_mutex_unlock(&connection->aliveMutex);
    Pthread_join(checkMsgsThread, NULL);
    Pthread_mutex_destroy(&connection->aliveMutex);
}*/

/*
void checkOfflineMessages(User *user, SendBuffer *sendBuffer, int eventfd)
{
    uint64_t eventfdCount;
    read(eventfd, &eventfdCount, sizeof eventfdCount);
    while (eventfdCount > 0) {
        checkMessages2(&connection)
        read(pollfds[msgsPoll].fd, &eventfdCount, sizeof eventfdCount);
    }
}*/

// return type makes this more complicated than I wanted. Command even more so.
// But the command has to be set just before sending so probably better this way.
// (Or change how command works). I could normally start filling the buffer at buffer+1, and command can be set at any time,
// which would set buffer[0]. If command is set at some point, then Send() will start at buffer[0], else at buffer[1]
// if command not 0, value will be added to end after null byte
/*ssize_t sendBuffer(Connection *connection, char command)
{
    ssize_t result = 0; // a bit dishonest returning this when maybe the send() would have returned error. This is not a problem now I'm using Send()?
    assert(connection->sendBuffer.sendBufferSize != 0);
    if (connection->sendBuffer.sendBufferSize != 0) {
        if (command != 0) {
            char str[2] = {command, 0};
            appendBuffer(&connection->sendBuffer, str);
            // below version was for command byte after null byte, which creates problems with differentiating between multiple messages received at once at client
            //connection->sendBuffer.sendBuffer[connection->sendBuffer.sendBufferSize+1] = command;
            // sendBuffer[sendBufferSize] is the null byte
            //connection->sendBuffer.sendBufferSize += 1; // bit weird because now sendBufferSize includes the null byte
            // and the +1 below will refer to the extra byte after the null byte
        }
        // unintended behaviour of strlcat2: If I give it an empty string, it will append a null byte, and won't increment the buffer size
        // that works out, so I'll use it to add the second null byte
        appendBuffer(&connection->sendBuffer, ""); // check this in gdb !!
        result = Send(connection->client->sockfd, connection->sendBuffer.sendBuffer, connection->sendBuffer.sendBufferSize + 2, 0);
        connection->sendBuffer.sendBufferSize = 0;
        connection->sendBuffer.sendBuffer[0] = 0;
    }
    return result;
}*/

// instead of setNotLoggedIn() + sendBuffer():
// would I even need a null termination in this case?
/*
void sendBuffer(char command)
{
    if (command != 0) {
        char str[2] = {LOGIN, 0};
        appendBuffer(str);
        // this would assume sufficient space:
        //sendData[sendDataSize] = command;
        //sendData[++sendDataSize] = 0;
    }
    send(clientSockfd, sendData, sendDataSize + 1, 0);
    sendDataSize = 0;
}*/

// name too long
// another way???? Doesn't really make sense to call this from sendWelcome
// set last byte instead of first seems easier

/*
void setBufferNotLoggedIn(SendBuffer *sendBuffer)
{
    char str[2] = {LOGIN, 0};
    appendBuffer(sendBuffer, str);
    //appendBuffer0(); // !! bad!? but normal appendBuffer uses strings so won't accept starting with null byte.
    // I could just use any other non-ascci character, here is all 1s // still seems stupid !!
    //appendBuffer("\255"); // "\.." notation actually uses octal!
    // ALSO: this system is assuming the buffer is empty, because the 255 char needs to go at the beginning of the buffer
    // but I wanted to not assume that from the functions calling this!!
}*/

/*
void setBufferLoggedIn()
{
}*/

// !! maybe I shouldn't provide sendBuffer but a string for the data to be put into?
// thread safety here? retrieveMessage retrieves the head
// since other people will only be adding to the tail, nothing else will change the head
// BUT a new message could be added after retrieveMessage returns 'no more messages' and before clearMessage() is called
// wait no because if retrieveMessage returns no more messages, clearMessage() won't be called
// other edge case would be if retrieveMessage returns the last message ?
// I don't think need any locks on this function (but do need on deeper layers of retrieve and clear)
// !! this function needs to be reworked or removed
/*
void checkMessages(User *user, SendBuffer *sendBuffer)
{
    char *sender;
    char *msg;
    MessageType messageType;
    char first = 1;

    while (hasMessages(user) == TRUE) { // because retrieveMessage() will block if no messages available
        if (first) { appendBuffer(sendBuffer, "You received messages while offline:\n", 0); first = 0; }
        //char str[MAXLENGTH] = {0};
        retrieveMessage(user, &sender, &msg, &messageType);
        bufferMessage(sendBuffer, sender, msg, messageType);
        clearMessage(user);
    }
}*/
