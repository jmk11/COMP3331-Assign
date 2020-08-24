#ifndef PROJECT_MESSAGE_H
#define PROJECT_MESSAGE_H

#include <semaphore.h>
#include "user.h"
//#include "constants.h"

typedef struct Node Node;

// should this even be public?
typedef struct messageList messageList;
struct messageList
{
    Node *head; // oldest
    Node *tail; // newest
    sem_t messageCount;
    pthread_mutex_t mutex;
    int eventfd; // names
};

void freeMessages(Node *node);
//void freeMessage(Message *message);
void pushMessage(messageList *list, const User *sender, User *receiver, const char *msg, MessageType messageType);
bool getMessage(messageList *list, User **sender, char **content, MessageType *messageType);
//void pushMessage(messageList *list, Message message);
void popMessage(messageList *list);


#endif //PROJECT_MESSAGE_H
