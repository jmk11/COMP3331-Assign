#ifndef PROJECT_BLOCKEDUSERS_H
#define PROJECT_BLOCKEDUSERS_H

#include "user.h"

typedef struct blockedUserList BUList;

void freeBlockedUsers(BUList *list);
bool pushUser(BUList *list, const User *blockee);
void removeUser(BUList *list, const User *blockee);
bool checkBlocked(BUList *list, const User *blockee);
BUList *createBUList(void);

#endif //PROJECT_BLOCKEDUSERS_H
