#ifndef PROJECT_MESSAGE_H
#define PROJECT_MESSAGE_H

#include <semaphore.h>

#include "user.h"

typedef struct Node Node;

// should this even be public?
typedef struct messageList messageList;
struct messageList {
    Node *head; // oldest
    Node *tail; // newest
    sem_t messageCount;
    pthread_mutex_t mutex;
    int eventfd; // names
};

void freeMessages(Node *node);
void pushMessage(messageList *list, const User *sender, User *receiver, const char *msg, MessageType messageType);
void getMessage(messageList *list, const User** sender, const char** content, MessageType *messageType);
void popMessage(messageList *list);

#endif //PROJECT_MESSAGE_H
