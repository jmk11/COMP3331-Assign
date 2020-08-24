#ifndef PROJECT_SERVER_H
#define PROJECT_SERVER_H

#include "clients.h"
#include "user.h"

// some of this should be private

#define USERSFILENAME "credentials.txt"

typedef struct UserList UserList;
// stupid name UserList vs userList
struct UserList {
    User *user;
    UserList *next;
};

// BAD!!! NOT HERE
//userList *users; // pointer or no? doesn't change
//User *userList;
//UserList *userList;
extern UserList *userList; // used in serverClients.c and commands.c
//User *user; // remove???

User *findUser(const UserList *userList, const char *username);
// I think it makes sense that the server should manage the list of users

// this shouldn't be accessible to commands.c but it should be accessible to serverClients -> ?? where to put
extern unsigned long timeout;       // serverClients.c, once
extern unsigned long blockDuration; // used in serverClients.c, once
extern char *welcome;               // used in serverClients.c, once
extern ClientsList *clientsList;    // This is being used in serverClients.c but I would rather it is dealt with in server.c
// but I would need to keep track of all threads and join them when they close, and then remove them
// but how would i do that? I don't know what order they'll close in so couldn't do blocking calls
// can't do non-blocking because would use all CPU just repeatedly checking
// unless checked once every second or something but I would need a whole separate thread to do that
// maybe that would be okay
// or I could send a message like eventfd to the main thread to indicate that a thread has finished and its resources thing can be freed
// or I could not free them at all..
// with eventfd how would I tell the server which thread closed? Unless I made one for every thread
// maybe some kind of message queue that returns threadid of finished thread

#endif //PROJECT_SERVER_H
