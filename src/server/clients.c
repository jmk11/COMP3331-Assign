/*
Linked list ADT of all clients currently connected to the server -
user struct pointer, IP, port, socket fd.
removal is done by matching a detail.

Concurrency:
only one thread will be adding or removing clients (main thread) (I think? !!!)
Other threads will keep pointers to particular elements of the list, and also traverse and read the list

It's assumed that the reading thread is terminated before 
a client node is removed - so noone will try to read a node that has been deleted

Duplicate checking:
no duplicate check because what would
I check for? I mean I could check if addr and sockfd are duplicates. but I
don't know how that could happen? It's fine and intended for multiple
connected clients to share an IP address would it be possible for the same IP
and port to try to connect twice?
*/

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>

#include "clients.h"
#include "common/helpers.h"
#include "user.h"

// maybe I don't really want this to be private?
// Not worth it? Don't want to call getSockfd() etc. all the time.
// Next can be private but could make different Client struct of user, addr,
// sockfd and make it public

// !! rwlock. Or maybe not? Might not be needed

typedef struct ClientNode ClientNode;
struct ClientNode {
    Client client;
    // thread?
    ClientNode *next;
};

// I'm making this ADT even more private
// or maybe not. because they know what a clientNode is
// the list portion is private
struct ClientsList {
    ClientNode *head;
    ClientNode *tail;
    pthread_rwlock_t rwlock;
    // I'm not going to do any duplication checks for this one?
};

void freeClientNodeR(ClientNode *node);
Client *findClientByUser(ClientNode *clientNode, const User *user);

ClientsList *createClientsList() {
    ClientsList *list = Malloc(sizeof(ClientsList));
    list->head = NULL;
    list->tail = NULL;
    pthread_rwlockattr_t attr;
    Pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
    // what does this attr setting actually do? I'm not sure it's what I want.
    // PTHREAD_RWLOCK_PREFER_WRITER_N is 'ignored by glibc' This 'ensures the
    // application developer will not take recursive readlocks thus avoiding
    // deadlocks.' I don't know what that means.
    Pthread_rwlock_init(&list->rwlock, &attr);
    return list;
}

void freeClientsList(ClientsList *list) {
    freeClientNodeR(list->head);
    free(list);
    // assert(list->head == NULL && list->tail == NULL);
    // all connections should have been closed and clients removed already
}

void freeClientNodeR(ClientNode *node) {
    if (node != NULL) {
        free(node->next);
        free(node);
    }
}

// same as blockedUsers
/*
* Remove client from list
*/
void removeClientNode(ClientsList *list, Client *client) {
    if (list->head == NULL) {
        return NULL;
    }

    Pthread_rwlock_wrlock(&list->rwlock); // could possibly be moved to allow more parallelization
    // at least to the next line, inside the if
    // because only one thread could possibly remove a given node
    // so if this is the only node, then list->head could not change to null at
    // any time other than this thread doing it

    // if the head matches
    if (&list->head->client == client) {
        // if only one element:
        if (list->head->next == NULL) {
            // assert(list->head == list->tail);
            list->tail = NULL;
        }
        ClientNode *head = list->head;
        list->head = list->head->next;
        free(head);
    } else {
        bool found = false;
        ClientNode *node;
        ClientNode *prev = list->head;
        for (node = list->head->next; found == false && node->next != NULL; node = node->next) {
            if (&node->client == client) {
                prev->next = node->next;
                free(node);
                found = true;
            }
            prev = node;
        }
        // if the tail matches
        if (found == false) {
            // assert(node == list->tail); // will be false if tried to remove client that isn't in list
            if (&node->client == client) {
                prev->next = node->next;
                list->tail = prev;
                free(node);
            }
        }
    }
    Pthread_rwlock_unlock(&list->rwlock);
}

/*
 * Returns pointer to malloced added client struct
 */
// better to pass pointer to addr?
// because for this, the bytes are copied for the arg, then copied from arg into heap
Client *pushClient(ClientsList *list, Client client) {
    // the list is global, many threads could read or write at once
    ClientNode *newNode = Malloc(sizeof(*newNode));
    newNode->next = NULL;
    newNode->client = client;

    Pthread_rwlock_wrlock(&list->rwlock);
    if (list->head == NULL) {
        // assert(list->tail == NULL);
        list->head = newNode;
        list->tail = newNode;
    } else {
        // assert(list->tail != NULL);
        list->tail->next = newNode;
        list->tail = newNode;
    }
    Pthread_rwlock_unlock(&list->rwlock);

    return &newNode->client;
}

/*
* Set ipaddr and port to IP address and port of current connection of user user.
* And return true on success, false on failure.
*/
bool getUserAddress(ClientsList *list, const User *user, uint32_t *ipaddr, unsigned short *port) {
    Pthread_rwlock_rdlock(&list->rwlock);
    Client *clientStruct = findClientByUser(list->head, user);
    if (clientStruct == NULL) {
        return false;
    }
    *ipaddr = clientStruct->ipaddr;      // clientStruct->addr.sin_addr.s_addr; 
                                         // not sure if this is good. Network byte order I think.
    *port = clientStruct->p2pserverport; // clientStruct->addr.sin_port;
                                         // network byte order must be, because htons
    Pthread_rwlock_unlock(&list->rwlock);

    return true;
}

Client *findClientByUser(ClientNode *clientNode, const User *user) {
    if (clientNode != NULL) {
        if (clientNode->client.user == user) {
            return &clientNode->client;
        }
        return findClientByUser(clientNode->next, user);
    }
    return NULL;
}
