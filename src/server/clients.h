#ifndef PROJECT_CLIENTS_H
#define PROJECT_CLIENTS_H

#include <netinet/in.h>

#include "user.h"

typedef struct ClientsList ClientsList;
typedef struct Client Client;
struct Client {
    User *user;
    uint32_t ipaddr;
    int sockfd;
    unsigned short p2pserverport; // network byte order
};

ClientsList *createClientsList();
void freeClientsList(ClientsList *list);
Client *pushClient(ClientsList *list, Client client);

void removeClientNode(ClientsList *list, Client *client); // weird

bool getUserAddress(ClientsList *list, const User *user, uint32_t *ipaddr,
                    unsigned short *port);

#endif // PROJECT_CLIENTS_H
