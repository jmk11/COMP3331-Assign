#ifndef PROJECT_P2PCONNECTIONS_H
#define PROJECT_P2PCONNECTIONS_H

#include "common/constants.h"

typedef struct p2pNode p2pNode;
typedef p2pNode p2pList;
/*struct p2pList
{
    p2pNode *head;
    //pthread_rwlock_t rwlock; // the client application only has one thread so no locking stuff needed
};*/

p2pNode *createp2pList();
void freep2pList(p2pNode *head);
bool addUser(p2pNode **head, const char *partnerName);
bool removeUser(p2pNode **head, const char *partnerName);
bool setsockfd(p2pNode *head, const char *partnerName, int sockfd);
char *findUserName(p2pNode *head, int sockfd);
int getsockfd(p2pNode *head, const char *username);

#endif //PROJECT_P2PCONNECTIONS_H
