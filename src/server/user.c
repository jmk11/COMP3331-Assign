/*
Defines and manages a user struct, and operations on a user's session.

When a new client requests for a connection, the server should prompt the
user to input the username and password and authenticate the user.

If the credentials are correct, the client is considered to be logged in (i.e. online) and a welcome message is displayed.
When all messaging is done, a user should be able to logout from the server.
On entering invalid credentials, the user is prompted to retry. After 3 consecutive failed attempts, the
user is blocked for a duration of block_duration seconds (block_duration is a command line argument
supplied to the server) and cannot login during this duration (even from another IP address). While a
user is online, if someone uses the same username/password to log in (even from another IP address),
then this new login attempt is denied
*/

//#include <stddef.h> // I don't remember ever #including this before?????
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include <time.h>

#include "blockedusers.h"
#include "common/helpers.h"
#include "message.h"
#include "user.h"
//#include "p2prequests.h"
//

// PRIVATE:
//User *findUser(User *userList, char *username);
//void initialiseUser(User *user, const char *username, const char *password);
//char notBarred(User *user, unsigned long blockDuration);
bool barred(User *user, unsigned long blockDuration);

// rather than passing around user pointers, I could have a users array and pass around indexes

// does it have to be 8 byte or 4 byte aligned?
// this would have 6 bytes of padding between loggedIn and blockedUsers?
// so failedAttempts and loggedIn may as well be ints?
// maybe do separate structs like Message?
struct User {
    char *username;
    char *password;
    char failedAttempts;
    bool loggedIn;   // boolean: or could implement as list of logged in users in Server
    time_t barredAt; // blocked?: time of blocking in unix seconds. Checked when user tries to log in. If enough seconds have passed, set back to 0?
    time_t lastSeen;
    messageList messages; // for waiting messages - sent while user offline
    //BUNode *blockedUsers; // list of users they blocked !!!! PROBLEM // I don't remember what the problem was
    BUList *blockedUsers;
    //p2pRList p2prequests;
};

// manual says malloc is MT-safe but I'm not sure exactly what that means
// is malloc atomic? Do I need to put a lock on this somewhere so two threads don't get overlapping or the same chunks?
User *createUser(const char *username, const char *password) {
    User *user = Malloc(sizeof(User));

    user->loggedIn = FALSE;
    user->barredAt = 0;
    user->failedAttempts = 0;
    user->lastSeen = 0; // or should I set here?

    user->username = Malloc(strlen(username) + 1); // !! CHECK MATH. ??: CAST TO ADDR BEFORE DOING POINTER ARITHMETIC ??????
    strcpy(user->username, username);              // !! CHECK
    user->password = Malloc(strlen(password) + 1);
    strcpy(user->password, password);

    //user->blockedUsers = NULL;
    //user->blockedUsers.head = NULL;
    //Pthread_rwlock_init(&user->blockedUsers.rwlock, NULL);
    user->blockedUsers = createBUList();

    user->messages.head = NULL;
    user->messages.tail = NULL;
    Sem_init(&(user->messages.messageCount), 0, 0); // do I need this semaphore with the eventfd? It's not atomic tho
    Pthread_mutex_init(&user->messages.mutex, NULL);
    user->messages.eventfd = Eventfd(0, EFD_SEMAPHORE); // not sure if the passing around of this breaks encapsulation. Possibly badly.
    // especially since the writing occurs here but the reading in server.c

    //user->p2prequests.head = NULL;
    //Pthread_mutex_init(&user->p2prequests.mutex, NULL);
    //user->p2prequests.eventfd = Eventfd(0, EFD_SEMAPHORE);

    return user;
}

// don't have two threads call this!
// but shouldn't happen anyway. Only main thread calls it at end of server running
// and actually it never gets run because server is shut down by ctrlc
void freeUser(User *user) {
    if (user != NULL) {
        //Pthread_rwlock_destroy(&user->blockedUsers.rwlock);
        freeBlockedUsers(user->blockedUsers);

        sem_destroy(&(user->messages.messageCount)); // this right?? // wrapper?
        Pthread_mutex_destroy(&user->messages.mutex);
        Close(user->messages.eventfd); // is this right? should I close?
        freeMessages(user->messages.head);

        //Pthread_mutex_destroy(&user->p2prequests.mutex);
        //Close(user->p2prequests.eventfd);
        //freep2pRList(user->p2prequests.head);

        free(user->password);
        free(user->username);
        free(user);
    }
}

//char login(User *userList, char *username, char *password, unsigned long blockDuration, User **loggedinUser)
char login(User *user, const char *password, unsigned long blockDuration) // I don't like removing loggedInUser.. feels wrong to just assume that the user is now logged in based on returned number
{
    //User *user = findUser(userList, username);
    //*loggedinUser = NULL;
    if (user == NULL) {
        return INVALIDUSERNAME;
    } // should this be here? or in find user?
    if (isLoggedIn(user) == TRUE) {
        return LOGGEDIN;
    }
    if (barred(user, blockDuration) == TRUE) {
        return BARRED;
    }
    if (strcmp((user)->password, password) == 0) {
        user->loggedIn = TRUE;
        user->failedAttempts = 0;
        //*loggedinUser = user;
        return LOGINSUCCESSFUL;
    } else {
        user->failedAttempts++;
        if (user->failedAttempts >= MAXATTEMPTS) {
            struct timespec ts;
            Clock_gettime(CLKID, &ts);
            //GETTIME(&ts);
            //getTime(&ts);
            user->barredAt = ts.tv_sec;
            return NOWBARRED;
            // !! is sec precise enough? or need for pass of blockDuration*billion nanoseconds?
        }
        return INCORRECTPASSWORD;
    }
}

// what if someone on another computer logs in to same user while this user is being barred?
// if someone tries to log in while this user is being unbarred, then that means they are trying to log in while that user
// is trying to log in elsewhere. That is a problem to deal with elsewhere
// YES, A PROBLEM: if it gets past the loggedIn check but doesn't get to set the loggedIn variable
// and someone on another computer logs in -> two people logged in to same user
// so need to lock, rw: I think only write once the password is determined as correct? else read?
// except maybe you could somehow get an extra attempt if you log in on 2 computers at same time? is that so bad?
// but is it really limited to just one attempt?
bool barred(User *user, unsigned long blockDuration) {
    if (user->barredAt == 0) {
        return FALSE; // free. !! or 0? is it yes barred or yes not barred????
    } else {
        struct timespec ts;
        Clock_gettime(CLKID, &ts);
        //getTime(&ts);
        if (ts.tv_sec >= user->barredAt + blockDuration) // types? // !! SEC V NANOSECOND
        {
            user->barredAt = 0; // unblock
            user->failedAttempts = 0;
            return FALSE;
        } else {
            return TRUE;
        }
    }
}

//============================================
// MESSAGES

void addMessage(const User *sender, User *receiver, const char *msg, MessageType messageType) {
    if (sender != NULL && receiver != NULL) {
        pushMessage(&receiver->messages, sender, receiver, msg, messageType);
        //uint64_t add = 1; // message
        //write(receiver->messages.eventfd, &add, sizeof add); // adds 1 to the eventfd counter
        // !! maybe the eventfd writing should occur in server.c? or message.c for mutex?
    }
}

// takes receiver
// sets sender, content
// too many layers, server -> user -> message?
/*
 * returns the oldest message. Does not delete or change any messages.
 * Call clearMessage() to clear the oldest message to access the next messages.
 */
// not sure if this needs lock because will be retrieving from beginning,
// while others will only be writing to end
bool retrieveMessage(User *receiver, char **senderName, char **content, MessageType *messageType) {
    User *sender;
    bool result = getMessage(&receiver->messages, &sender, content, messageType);
    if (result != FALSE) {
        *senderName = sender->username;
    } else {
        //*senderName = NULL;
        //*content = NULL;
        // return 0 -> unchanged?
    }
    return result;
}

// many layers again
/*
 * Deletes the oldest message, allowing access to the next.
 */
void clearMessage(User *receiver) {
    popMessage(&receiver->messages);
}

// violates ADT?
// necessary? can use retrieveMessage
// read/write lock maybe
bool hasMessages(User *user) {
    return user->messages.head == NULL ? FALSE : TRUE;
}

//===================================================================

//===================================================================
// BLOCKED USERS

// readwrite (read) lock
bool isBlocked(const User *blocker, const User *blockee) {
    return checkBlocked(blocker->blockedUsers, blockee);
}

// readwrite (write) lock
bool blockUser(User *blocker, const User *blockee) {
    /*
    if (! isBlocked(blocker, blockee)) { // same situation as unblock user
        blocker->blockedUsers.head = pushUser(blocker->blockedUsers.head, blockee);
        return 1;
    }*/
    // pushUser already checks if user is already in the list
    return pushUser(blocker->blockedUsers, blockee);
}

// readwrite (write) lock
bool unblockUser(User *blocker, const User *blockee) {
    if (isBlocked(blocker, blockee) == TRUE) { // slow way of doing it - goes through list once to check if blocked and again to remove.
                                               // could change removeUser() so that could return whether or not blocked !!
        //blocker->blockedUsers = removeUser(blocker->blockedUsers, blockee);
        removeUser(blocker->blockedUsers, blockee);
        return TRUE;
    }
    return FALSE;
}

//======================================================================
// GETTERS AND SETTERS

void updateTimeActive(User *user) {
    struct timespec ts;
    Clock_gettime(CLKID, &ts);
    //getTime(&ts);
    user->lastSeen = ts.tv_sec;
}

// why not just leave it to crash if NULL is supplied?
time_t getLastSeen(const User *user) {
    return user->lastSeen;
}

// why did clion recommend this return const char? !
char *getUsername(const User *user) {
    return user->username;
}

// only update timeactive on logout
void logout(User *user) {
    struct timespec ts;
    Clock_gettime(CLKID, &ts);
    user->lastSeen = ts.tv_sec;
    user->loggedIn = FALSE;
}

bool isLoggedIn(const User *user) {
    return user->loggedIn;
}

int getMsgsEventfd(const User *user) {
    return user->messages.eventfd;
}

/*
int getRequestsEventfd(const User *user)
{
    return user->p2prequests.eventfd;
}*/

/*
char notBarred(User *user, unsigned long blockDuration) // !! NAME
{
    if (user->barredAt == 0) {
        return 1; // free. !! or 0? is it yes barred or yes not barred????
    }
    else {
        struct timespec ts;
        Clock_gettime(CLOCK_REALTIME, &ts);
        if (ts.tv_sec >= user->barredAt + blockDuration) // types? // !! SEC V NANOSECOND
        {
            user->barredAt = 0; // unblock
            user->failedAttempts = 0;
            return 1;
        }
        else {
            return 0;
        }
    }
}*/

/*if (notBarred(user, blockDuration)) // !! rename to barred?
    {
        if (strcmp((user)->password, password) == 0) {
            (user)->loggedIn = 1;
            (user)->failedAttempts = 0;
            //(*loggedinUser) = user;
            return LOGINSUCCESSFUL;
        }
        else {
            (user)->failedAttempts++;
            if ((user)->failedAttempts >= MAXATTEMPTS) {
                struct timespec ts;
                Clock_gettime(CLOCK_REALTIME, &ts);
                (user)->barredAt = ts.tv_sec;
                return NOWBARRED;
                // !! is sec precise enough? or need for pass of blockDuration*billion nanoseconds?
            }
            return INCORRECTPASSWORD;
        }
    }
    return BARRED;*/

//======================================================================
// P2P REQUESTS
/*
void addRequest(User *requestee, const User *requester, IP ip, unsigned short port)
{
    if (requestee != NULL && requester != NULL) {
        pushRequest(&requestee->p2prequests, requester, ip, port);
    }
}

// return success?
void retrieveRequest(User *requestee, char **requesterName, IP *ip, unsigned short *port)
{
    User *requester;
    getRequest(&requestee->p2prequests, &requester, ip, port);
    // checking?
    *requesterName = requester->username;
}

void clearRequest(User *requestee)
{
    popRequest(&requestee->p2prequests);
}*/