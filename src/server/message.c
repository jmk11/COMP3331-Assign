/*
Queue ADT (linked list) of a client's messages waiting for delivery 
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/eventfd.h> // for some reason this includes uint64_t

#include "common/helpers.h"
#include "message.h"

// queue ADT
// is it though?
// doesn't know what User is, and User doesn't know what it is

// private
typedef struct Message Message;
struct Message {
    MessageType messageType;
    const User *sender;
    // User *sender;
    User *receiver; // needed? because stored inside receiver
    char *content;
};

struct Node {
    Message message; // *void pointer?
    Node *next;
};
// this ends up exactly the same in memory as if the Message fields were just
// directly in Node, right?

void pushMessage2(messageList *list, Message message);

// messages read from head, added to tail, do I need a lock here?
// maybe just need to make sure that nothing is assigned to messages->head until
// the entire message is constructed
void pushMessage(messageList *list, const User *sender, User *receiver,
                 const char *msg, MessageType messageType) // 'message' name
// order of sender/receiver? it makes sense for it be different in User /
// message but that's crazy?
{
    if (sender != NULL &&
        list != NULL) { // return success or not?? // !! double check of sender
                        // & receiver not null, in User and here
        Message newMessage;
        newMessage.messageType = messageType;
        newMessage.receiver = receiver;
        newMessage.sender = sender;
        size_t msgLen = strlen(msg);
        newMessage.content = Malloc(msgLen + 1);
        strncpy(newMessage.content, msg, msgLen);
        newMessage.content[msgLen] = 0;

        pushMessage2(list, newMessage);
    }
}

// push message onto list
// NAME !!
// have User call this and then this call the struct creation method? !!
// two threads could call this at the same time
// could both be in list->tail == NULL section and one overwrites the other ->
// lost message probably? same problem with list->tail != NULL? if list->tail !=
// NULL, then the list has at least one element already thread A could set
// list->tail->next to be newNodeA, then thread B sets list->tail->next to be
// newNodeB then thread A sets list->tail to be newNodeA, thread B sets
// list->tail to be newNodeB so newNodeA has no references? so lock that part as
// well
void pushMessage2(messageList *list, Message message) // 'message' name
{
    Node *newNode = Malloc(sizeof(Node));
    newNode->next = NULL;
    newNode->message = message;

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

// blocks until message available
// I guess that means return can be removed
// only one thread will call this
// so won't clash with other readers but could clash with writers
// the wait + post thing seems bad, like it would need a mutex around it?
// and I'd like to have a mutex around eventfd changing but idk if necessary...
// only one thread will be calling get and pop on this list, so that can't
// happen at the same time clashing with writers: 1 message has to be available,
// because of semaphore so a new message added while this is being read will at
// worst change head->next so doesn't matter?
bool getMessage(messageList *list, User **sender, char **content,
                MessageType *messageType) {
    sem_wait(&(list->messageCount)); // !! replicates NULL check? Can remove
                                     // NULL check?
    // !! Sem_wait
    // I want this to only load a message if one is available
    // but I don't want it to decrement the amount, that should happen in
    // popMessage()
    // ...
    // for now I'm removing the wait (& decrement) from pop, assuming that a
    // message can only be got once !! meaning that it is 100% required to call
    // pop() after get() to have any sensible behaviour. that seems bad!! not
    // anymore, now I'm posting after waiting ... seems bad?

    if (list->head != NULL) {
        Message *message =
            &(list->head->message); // faster to use pointer than copy data ??
        *sender =
            message->sender; // !! how to specify this const stuff? I'm editing
                             // the value of *sender, but I won't change any
                             // memory based on the value of *sender
        *content = message->content;
        *messageType = message->messageType;
        sem_post(&list->messageCount);
        return TRUE;
    }
    return FALSE;
    // else { assert(0); } // because semaphore
    //*sender = NULL;
    //*content = NULL;
    //*messageType = NULL;
    // say that return 0 means values unchanged?
    // sem_post(&list->messageCount);
    // return 0;
    // the char return is not needed - can use nulls
    // but I don't want to have one of sender and content be returned and the
    // other returned through pointer stuff - not symmetrical
}

// maybe need to lock to make sure noone is adding while this is popping?
// yes. Because list->head->next could be null (ie only 1 message), but then by
// the time list->tail is set to null, something else could have been put in
// list tail that is now being removed but maybe only need to mutex that
// section? if 1 message in second part: list->head->next may be retrieved to be
// NULL, but then a new message is added and then this sets list->head to NULL.
// The added message is lost ie: list->head->next changes between moving into
// register and moving from register into list->head so lock whole process.
// frees don't matter because this function is the only thing that will have a
// pointer to the chunks by then unless free() is not really safe in general?
void popMessage(messageList *list) {
    sem_wait(&(list->messageCount)); // !! replicates NULL check? Can remove
                                     // NULL check?
    // actually, it should throw some kind of error, or just return, if there is
    // no message to pop but need to decrement count

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
    read(list->eventfd, &eventfdCount,
         sizeof eventfdCount); // I don't really want the count, just to
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

/*
 * // instead of doing getMessage() and clearMessage() separately
char getMessage(messageList *list, Message *message)
{
    if (list->head != NULL) {
        *message = list->head->message;
        //memcpy(message, (void*) &node->message, sizeof(Message));
        return 1;
    }
    return 0;
}*/
