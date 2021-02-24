/*
Queue ADT (linked list) of a client's messages waiting for delivery 

Concurrency:
Multiple threads could be adding messages to the tail at the same time.
One thread will be getting and popping messages from the head.

Messages are retrieved with getMessage(), and must then later be popped & freed with popMessage()
This is to avoid having to copy the message when retrieving it.
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <sys/eventfd.h> // for some reason this includes uint64_t

#include "common/helpers.h"
#include "message.h"

// queue ADT
// is it though?
// doesn't know what User is, and User doesn't know what it is

typedef struct Message Message;
struct Message {
    MessageType type;
    const User *sender;
    // User *sender;
    User *receiver; // needed? because stored inside receiver
    char *content; // const except when it's freed so not const
};

struct Node {
    Message message; // *void pointer?
    Node *next;
};

// messages read from head, added to tail, do I need a lock here?
// maybe just need to make sure that nothing is assigned to messages->head until
// the entire message is constructed
/*
* Add message to tail of list
*/
void pushMessage(messageList *list, const User *sender, User *receiver, const char *msg, MessageType messageType) { // 'message' name
// order of sender/receiver? it makes sense for it be different in User / message but that's crazy?
    if (sender != NULL && list != NULL) { 
        // !! double check of sender & receiver not null, in User and here

        Node *newNode = Malloc(sizeof(Node));
        newNode->next = NULL;
        newNode->message.type = messageType;
        newNode->message.content = strdup(msg);
        newNode->message.receiver = receiver;
        newNode->message.sender = sender;

        Pthread_mutex_lock(&list->mutex);
        if (list->tail == NULL) {
            // assert(list->head == NULL);
            list->head = newNode;
            list->tail = newNode;
        } else {
            // assert(list->head != NULL);
            list->tail->next = newNode;
            list->tail = newNode;
        }
        sem_post(&(list->messageCount)); // may as well put inside mutex?
        // should do the eventfd incrementation inside mutex?
        uint64_t add = 1;                       // message
        write(list->eventfd, &add, sizeof add); // adds 1 to the eventfd counter
        Pthread_mutex_unlock(&list->mutex);
    }
}

// the wait + post thing seems bad, like it would need a mutex around it?
// and I'd like to have a mutex around eventfd changing but idk if necessary...
// only one thread will be calling get and pop on this list, so that can't
// happen at the same time clashing with writers: 1 message has to be available,
// because of semaphore so a new message added while this is being read will at
// worst change head->next so doesn't matter?
/*
* Retrieve message from head of list, but do not remove it from list.
* Blocks until a message is available and sets content and messageType
* Once you are finished with the message's content or have copied it, call popMessage() to free and remove from the list.
*/
void getMessage(messageList *list, const User **sender, const char **content, MessageType *messageType) {
    // I'm quite sure that User** const sender is wrong use
    sem_wait(&(list->messageCount)); 
    // !! Sem_wait

    Message *message = &(list->head->message); // faster to use pointer than copy data ??
    *sender = message->sender; // !! how to specify this const stuff? I'm editing
                            // the value of *sender, but I won't change any
                            // memory based on the value of *sender
    *content = message->content;
    *messageType = message->type;
    sem_post(&list->messageCount);
}

/*
* Remove message from head of list.
*/
void popMessage(messageList *list) {
    // actually, it should throw some kind of error, or just return, if there is
    // no message to pop but need to decrement count
    // ??

    // assert(list->head != NULL);
    // assert(list->tail->next == NULL);

    Pthread_mutex_lock(&list->mutex);
    if (list->head->next == NULL) {
        // assert(list->head == list->tail);
        list->tail = NULL;
    }
    Node *node = list->head;
    list->head = list->head->next;
    uint64_t eventfdCount;
    read(list->eventfd, &eventfdCount, sizeof eventfdCount); // I don't really want the count, just to
                               // decrement it. Can I pass NULL?
    Pthread_mutex_unlock(&list->mutex);

    free(node->message.content);
    free(node);
    // or here?
}

// !! use this !! free stuff !!
void freeMessages(Node *node) {
    if (node != NULL) {
        freeMessages(node->next);
        free(node->message.content); // will content be malloced?
        free(node);
    }
}
