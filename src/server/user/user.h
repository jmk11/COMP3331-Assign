#ifndef PROJECT_USER_H
#define PROJECT_USER_H

#include <time.h>

#include "common/constants.h"
// maybe messageType should be defined here?

typedef struct User User;
/*struct UserList
{
   User user; // not possible here without User definition bc doesn't know how much memory to allocate etc. Would have to do with pointers
   userList *next;
};*/


//void freeUserList(User *userList);
//char login(User *userList, char *username, char *password, unsigned long blockDuration, User **user);
char login(User *user, const char *password, unsigned long blockDuration);
void logout(User *user);

bool hasMessages(User *user);
void addMessage(const User *sender, User *receiver, const char *msg, MessageType messageType);
bool retrieveMessage(User *receiver, char **senderName, char **content, MessageType *messageType);
void clearMessage(User *receiver);

bool blockUser(User *blocker, const User *blockee);
bool unblockUser(User *blocker, const User *blockee);

//User *findUser(User *userList, char *username); // public or no?

User *createUser(const char *username, const char *password);
void freeUser(User *user);

void updateTimeActive(User *user);

bool isBlocked(const User *blocker, const User *blockee);
bool isLoggedIn(const User *user);
time_t getLastSeen(const User *user);
char *getUsername(const User *user); // !! return const?

int getMsgsEventfd(const User *user);

#endif //PROJECT_USER_H
