/*
linked list ADT of a client's active P2P connections 
with duplication checking and removal based on username.
*/

#include <stdlib.h>
#include <string.h>

#include "common/constants.h"
#include "common/helpers.h"
#include "p2pConnections.h"

struct p2pNode {
    char *username;
    int sockfd;
    p2pNode *next;
};

void freep2pNode(p2pNode *node);

p2pNode *createp2pList() {
    /*
    p2pNode *head = Malloc(sizeof(*head));
    head->username = NULL;
    head->sockfd = 0;
    head->next = NULL;
    return head;*/
    return NULL;
}

void freep2pList(p2pNode *head) {
    if (head != NULL) {
        freep2pNode(head->next);
        free(head->username);
        free(head);
    }
}

void freep2pNode(p2pNode *node) {
    if (node != NULL) {
        free(node->username);
        free(node);
    }
}

// return TRUE if success, FALSE if user already in list
// sets *head to new head
bool addUser(p2pNode **head, const char *partnerName) {
    // have to go through whole list to check if user is already in it
    // so no point keeping tail
    p2pNode *newNode = Malloc(sizeof(*newNode));
    newNode->next = NULL;
    newNode->username = Malloc(strlen(partnerName) + 1); // remove const?
    newNode->sockfd = 0;
    strcpy(newNode->username, partnerName);

    if (head == NULL) {
        return false;
    }
    if (*head == NULL) {
        *head = newNode;
        return true;
    }
    p2pNode *cur = *head;
    for (; cur->next != NULL; cur = cur->next) {
        if (strcmp(cur->username, partnerName) == 0) {
            freep2pNode(newNode);
            return false;
        }
    }
    if (strcmp(cur->username, partnerName) == 0) {
        freep2pNode(newNode);
        return false;
    }
    // now cur is tail

    cur->next = newNode;
    return true;
}

// returns TRUE if found and removed, FALSE if not found
bool removeUser(p2pNode **head, const char *partnerName) {
    p2pNode *node;
    p2pNode *oldhead;
    p2pNode *prev;

    if (head != NULL && *head != NULL) {
        if (strcmp((*head)->username, partnerName) == 0) {
            // if head matches
            oldhead = *head;
            *head = (*head)->next;
            freep2pNode(oldhead);
            return true;
        } else {
            prev = *head;
            for (node = (*head)->next; node != NULL; node = node->next) {
                if (strcmp(node->username, partnerName) == 0) {
                    prev->next = node->next;
                    freep2pNode(node);
                    return true;
                    // break; // or set node = NULL to exit loop? //return head;
                }
                prev = node;
            }
        }
    }
    return false;
}

// returns TRUE if successfully set, FALSE if couldn't find username matching
// partnerName
bool setsockfd(p2pNode *head, const char *partnerName, int sockfd) {
    if (head != NULL) {
        if (strcmp(head->username, partnerName) == 0) {
            head->sockfd = sockfd;
            return true;
        }
        return setsockfd(head->next, partnerName, sockfd);
    }
    return false;
}

// returns 0 if sockfd not set
// returns 1 if user not in list
int getsockfd(p2pNode *head, const char *username) {
    if (head != NULL) {
        if (strcmp(head->username, username) == 0) {
            return head->sockfd;
        }
        return getsockfd(head->next, username);
    }
    return -1;
}

char *findUserName(p2pNode *head, int sockfd) {
    if (head != NULL) {
        if (head->sockfd == sockfd) {
            return head->username;
        }
        return findUserName(head->next, sockfd);
    }
    return NULL;
}

/*
bool checkBlocked(BUList *list, const User *blockee)
{
    bool found = FALSE;
    Pthread_rwlock_rdlock(&list->rwlock);
    for (BUNode *node = list->head; node != NULL && found == FALSE; node =
node->next) { if (node->user == blockee) { found = TRUE;
        }
    }
    // a mistake I made: I was just returning above without unlocking ->
deadlock
    // so you really want to avoid multiple returns when using locks
    Pthread_rwlock_unlock(&list->rwlock);
    return found;
}*/
