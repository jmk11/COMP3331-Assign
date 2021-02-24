// /*
// Thread-safe Singly Linked List ADT
// Provide a function (or NULL) for comparisons for adding/removing by a certain detail


// linked list ADT of all blocked users for a particular client
// but with duplication checking (by username) when adding.
// Removal is done by matching username.
// Each user has a list of blocked users.

// Concurrency:
// Only one thread will edit a particular list - the thread managing the blocking user
// But this editing could be done at the same time as reading by another thread.
// -> readwrite lock
// a reader could just miss a user that is about to be blocked - does that matter? 
// I'm assuming that the actual assignment into memory takes
// only one operation (not done in parts) which should be right but maybe I shouldn't assume that...

// Design:
// have to go through whole list to check if user is already in it, so no point keeping tail
// */

// #include <pthread.h>
// #include <stdbool.h>
// #include <stdlib.h>

// #include "blockedusers.h"
// #include "common/helpers.h"
// #include "linkedlist.h"

// typedef struct Node {
//     const void *data;
//     Node *next;
// } Node;

// struct List {
//     Node *head;
//     compareFn compare;
//     pthread_rwlock_t rwlock;
// };

// // typedef struct blockedUserNode BUNode;
// // struct blockedUserList {
// //     BUNode *head;
// //     pthread_rwlock_t rwlock;
// // };
// // struct blockedUserNode {
// //     const User *user;
// //     BUNode *next;
// // };

// void freeNode(Node *node);

// List *createList(compareFn compare) {
//     List *list = Malloc(sizeof(*list));
//     list->head = NULL;
//     list->compare = compare;
//     pthread_rwlockattr_t attr;
//     Pthread_rwlockattr_setkind_np(&attr, PTHREAD_RWLOCK_PREFER_WRITER_NONRECURSIVE_NP);
//     // what does this attr setting actually do? I'm not sure it's what I want.
//     // PTHREAD_RWLOCK_PREFER_WRITER_N is 'ignored by glibc' This 'ensures the
//     // application developer will not take recursive readlocks thus avoiding
//     // deadlocks.' I don't know what that means.
//     Pthread_rwlock_init(&list->rwlock, &attr);
//     return list;
// }

// void freeList(List *list) {
//     freeNode(list->head);
//     Pthread_rwlock_destroy(&list->rwlock); // did I read something about resetting attributes to null before destroying?
//     free(list);
// }

// void freeNode(Node *node) {
//     if (node != NULL) {
//         freeNode(node->next);
//         free(node);
//     }
// }

// bool addItem(List *list, void *item, )

// bool pushUser(BUList *list, const User *blockee) {
//     if (list == NULL) { // this should never happen. maybe better to just let it crash?
//         return false;
//     }

//     // find end of list, checking along that way that this insertion will not create a duplicate
//     BUNode *cur = list->head;
//     if (cur != NULL) {
//         for (; cur->next != NULL; cur = cur->next) {
//             if (cur->user == blockee) {
//                 return false;
//             }
//         }
//         if (cur->user == blockee) {
//             return false;
//         }
//     }

//     BUNode *newNode = Malloc(sizeof(BUNode));
//     newNode->next = NULL;
//     newNode->user = blockee;

//     Pthread_rwlock_wrlock(&list->rwlock); // this is the only place in the function that the list is actually edited
//     if (cur != NULL) {
//         cur->next = newNode;
//     } else {
//         list->head = newNode;
//     }
//     Pthread_rwlock_unlock(&list->rwlock);
//     return true;

//     // either keep track of prev, which is obv slower,
//     // or add more code to do while cur->next != NULL, and then an extra check at the end
// }

// // what does this comment mean?
// // recursively?
// // So the checkBlocked() function, running through blocked users
// // list, could run while this is running if (head == NULL) does nothing to list,
// // so don't need to lock for that if blockee is head, the read() function could
// // run after head has been freed but before the new head has been set so read()
// // will read freed memory. BAD, may have been overwritten by free bins stuff. so
// // lock but the actual head of the list doesn't get changed until the call in
// // User assigns it. Actually since it is now passing a list pointer instead of a
// // node, I will change the head in here Which also means that I could make it
// // change the head before it is freed. So the situation will never be dangerous?
// // A reader will never read data that has been freed
// // It could be possible for a reader to read a user as blocked when this is
// // about to free it, and when the message was sent after an unblock call but
// // that could happen anyway, and vice versa this could run an unblock call
// // before a message that was actually sent before the unblock call

// /*
// Remove blockee from list
// */
// void removeUser(BUList *list, const User *blockee) {
//     BUNode *node;
//     BUNode *head;

//     if (list->head != NULL) {
//         Pthread_rwlock_wrlock(&list->rwlock);
//         if (list->head->user == blockee) {
//             head = list->head;
//             list->head = list->head->next;
//             free(head);
//         } else {
//             BUNode *prev = list->head;
//             for (node = list->head->next; node != NULL; node = node->next) {
//                 if (node->user == blockee) {
//                     prev->next = node->next;
//                     free(node);
//                     break;
//                 }
//                 prev = node;
//             }
//         }
//         Pthread_rwlock_unlock(&list->rwlock);
//     }
// }

// /*
// Return true if blockee is blocked (in the list), false if not
// */
// bool checkBlocked(BUList *list, const User *blockee) {
//     bool found = false;
//     Pthread_rwlock_rdlock(&list->rwlock);
//     for (BUNode *node = list->head; node != NULL && found == false; node = node->next) {
//         if (node->user == blockee) {
//             found = true;
//         }
//     }
//     // a mistake I made: I was just returning above without unlocking -> deadlock
//     // so you really want to avoid multiple returns when using locks
//     Pthread_rwlock_unlock(&list->rwlock);
//     return found;
// }
