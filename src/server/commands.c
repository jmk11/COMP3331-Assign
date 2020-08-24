/*
Execute server-side logic for commands sent by clients
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "commands.h"
#include "common/helpers.h"
#include "common/protocol.h"
#include "server.h"

bool execMessage(char *args, Connection *connection);
bool execBroadcast(char *args, Connection *connection);
bool execWhoelse(Connection *connection);
bool execWhoelsesince(char *args, Connection *connection);
bool execBlock(char *args, Connection *connection);
bool execUnblock(char *args, Connection *connection);
// bool execLogout(char *arg, Connection *connection);
bool execIncorrectCommand();

bool execStartPrivate(char *argsinput, Connection *connection);
// bool execPrivate(char *argsinput, Connection *connection);
// bool execStopPrivate(char *argsinput, Connection *connection);
bool execSetupP2P(char *argsinput, Connection *connection);

void sendP2PFail(Connection *connection, char *failedName);

// CHECK THAT THE ARGS ARE THERE!
bool execMessage(char *argsinput, Connection *connection) {
    unsigned char numArgs = 2;
    char *args[numArgs];
    parseRequest(argsinput, numArgs, args);
    char *recipient = args[0];
    char *msg = args[1]; // if 2 zeroes in a row, msg would be NULL

    if (recipient == NULL || msg == NULL) {
        appendBuffer(&connection->sendBuffer, "Invalid arguments.\n", 0);
    } else {
        User *recipientUser = findUser(userList, recipient);
        if (recipientUser == NULL) {
            appendBuffer(&connection->sendBuffer, "Recipient user does not exist.\n", 0);
        } else if (recipientUser == connection->client->user) {
            appendBuffer(&connection->sendBuffer, "You can't message yourself!\n", 0);
        } else if (isBlocked(recipientUser, connection->client->user) == true) { // did recipient block user
            char str[MAXLENGTH];
            snprintf(str, MAXLENGTH, "You can't message %s as they have blocked you.\n", recipient);
            appendBuffer(&connection->sendBuffer, str, 0);
        } else {
            addMessage(connection->client->user, recipientUser, msg, USER_MESSAGE);
        }
    }

    return true;
}

// somehow send a message to all other threads?
// or just put something in shared memory for them to read when ready?
// This is why I wanted a general 'server' thread - threadA would tell the
// server thread that it wants to broadcast msg and the server thread would send
// a message to all the client-serving threads to send the msg
bool execBroadcast(char *argsinput, Connection *connection) {
    /*unsigned char numArgs = 1;
    char *args[numArgs];
    parseRequest(argsinput, numArgs, args);
    char *msg = args[0];*/
    // check num of args: maybe checkNumArgs(args, argNum) function?

    char *msg = argsinput;
    bool complete = broadcast(connection->client->user, msg, USER_MESSAGE);
    if (complete == false) {
        appendBuffer(&connection->sendBuffer, "Your message could not be delivered to some recipients\n", 0);
    }

    return true;
}

// maybe need lock here
// if implement as reading through userList to see who is loggedIn, then the
// list won't change, only the loggedIn variable sounds okay? if another user
// logs in as this is executing then it seems fair that they may or may not be included
bool execWhoelse(Connection *connection) {
    // no args
    // go through users for loggedIn users
    bool found = false;
    // mutex on users being logged in?
    for (UserList *cur = userList; cur != NULL; cur = cur->next) {
        if (cur->user != connection->client->user && (isLoggedIn(cur->user) == true)) {
            appendBuffer(&connection->sendBuffer, getUsername(cur->user), 0);
            appendBuffer(&connection->sendBuffer, "\n", 0);
            found = true;
        }
    }
    if (found == false) {
        appendBuffer(&connection->sendBuffer, "Noone.\n", 0);
    }

    return true;
}

bool execWhoelsesince(char *argsinput, Connection *connection) {
    unsigned char arrLength = 2;
    char *args[arrLength];
    parseRequest(argsinput, arrLength, args);
    char *secondsS = args[0];
    if (secondsS == NULL) {
        appendBuffer(&connection->sendBuffer, "Invalid arguments.\n", 0);
    } else {
        unsigned long seconds = strtoul(secondsS, NULL, 10);
        // NEED CHECK FOR THIS !! what if they type a letter etc.

        struct timespec ts;
        Clock_gettime(CLKID, &ts);
        time_t sinceTime = ts.tv_sec - seconds; // !! should it just be -seconds ??
        // get linked list? of users active since that time
        // loop through userList? but would need to redefine userList so next
        // field was accessible dTODO: CHANGE THIS TO PROPER WAY!! get all
        // online, same as whoelse, and then get all active since time
        bool found = false;
        for (UserList *cur = userList; cur != NULL; cur = cur->next) {
            if ((cur->user != connection->client->user) &&
                ((isLoggedIn(cur->user) == true) || ((getLastSeen(cur->user) >= sinceTime) && (getLastSeen(cur->user) > 0)))) {
                appendBuffer(&connection->sendBuffer, getUsername(cur->user), 0);
                appendBuffer(&connection->sendBuffer, "\n", 0);
                found = true;
            }
        }
        if (found == false) {
            appendBuffer(&connection->sendBuffer, "Noone\n", 0);
        }
    }

    return true;
}

bool execBlock(char *argsinput, Connection *connection) {
    unsigned char arrLength = 2;
    char *args[arrLength];
    parseRequest(argsinput, arrLength, args); // first word in args[0], rest in args[1]
    char *blockee = args[0];
    // messy way of saying null-terminate after first word!!

    // char *blockee = argsinput; // but probably just want first word

    // same as message - generalise somehow? Just need to change text "message" to "blocked"
    if (blockee == NULL) {
        appendBuffer(&connection->sendBuffer, "Invalid arguments.\n", 0);
    } else {
        User *blockeeUser = findUser(userList, blockee);
        if (blockeeUser == NULL) {
            char str[MAXLENGTH];
            snprintf(str, MAXLENGTH, "User %s does not exist and cannot be blocked.\n", blockee);
            appendBuffer(&connection->sendBuffer, str, 0);
        } else if (blockeeUser == connection->client->user) {
            appendBuffer(&connection->sendBuffer, "You can't block yourself!\n", 0);
        } else {
            if (blockUser(connection->client->user, blockeeUser) == false) {
                appendBuffer(&connection->sendBuffer, "You have already blocked ", 0);
                appendBuffer(&connection->sendBuffer, blockee, 0);
                appendBuffer(&connection->sendBuffer, "!\n", 0);
            } else {
                appendBuffer(&connection->sendBuffer, blockee, 0);
                appendBuffer(&connection->sendBuffer, " is blocked.\n", 0);
            }
        }
    }

    return true;
}

// Another thread could attempt to read the blocked list while this one is
// editing it
bool execUnblock(char *argsinput, Connection *connection) {
    unsigned char arrLength = 2;
    char *args[arrLength];
    parseRequest(argsinput, arrLength,
                 args); // first word in args[0], rest in args[1]
    char *blockee = args[0];

    // char *blockee = argsinput; // prob just want first word

    // !! similar to block! generalise?
    if (blockee == NULL) {
        appendBuffer(&connection->sendBuffer, "Invalid arguments.\n", 0);
    } else {
        User *blockeeUser = findUser(userList, blockee);
        if (blockeeUser == NULL) {
            char str[MAXLENGTH];
            snprintf(str, MAXLENGTH,
                     "User %s does not exist and cannot be unblocked.\n",
                     blockee);
            appendBuffer(&connection->sendBuffer, str, 0);
        } else if (blockeeUser == connection->client->user) {
            appendBuffer(&connection->sendBuffer,
                         "You can't unblock yourself!\n", 0);
        } else {
            if (unblockUser(connection->client->user, blockeeUser) == false) {
                appendBuffer(&connection->sendBuffer, "Cannot block ", 0);
                appendBuffer(&connection->sendBuffer, blockee, 0);
                appendBuffer(&connection->sendBuffer,
                             " as they were not blocked in the first place.\n",
                             0);
                // is this split thing unclear? should do snprintf?
            } else {
                appendBuffer(&connection->sendBuffer, blockee, 0);
                appendBuffer(&connection->sendBuffer, " is unblocked.\n", 0);
            }
        }
    }

    return true;
}

bool execStartPrivate(char *argsinput, Connection *connection) {
    unsigned char numArgs = 2;
    char *args[numArgs];
    parseRequest(argsinput, numArgs, args);
    char *partnerName = args[0]; // single word

    if (partnerName == NULL) {
        appendBuffer(&connection->sendBuffer, "Invalid arguments.\n", 0);
    } else {
        User *partnerUser = findUser(userList, partnerName);
        if (partnerUser == NULL) {
            sendP2PFail(connection, partnerName);
            char str[MAXLENGTH];
            snprintf(str, MAXLENGTH, "Partner user %s does not exist.\n",
                     partnerName);
            appendBuffer(&connection->sendBuffer, str, 0);
        } else if (partnerUser == connection->client->user) {
            sendP2PFail(connection, partnerName);
            appendBuffer(&connection->sendBuffer,
                         "You can't start a P2P connection with yourself!\n",
                         0);
        } else if (isBlocked(partnerUser, connection->client->user) == TRUE) {
            sendP2PFail(connection, partnerName);
            char str[MAXLENGTH];
            snprintf(str, MAXLENGTH,
                     "You can't connect with %s as they have blocked you.\n",
                     partnerName);
            appendBuffer(&connection->sendBuffer, str, 0);
        } else if (isLoggedIn(partnerUser) == FALSE) {
            sendP2PFail(connection, partnerName);
            char str[MAXLENGTH];
            snprintf(str, MAXLENGTH,
                     "You can't connect with %s as they are offline.\n",
                     partnerName);
            appendBuffer(&connection->sendBuffer, str, 0);
        }
        // else if already connected - maybe ask client if they are connected to
        // them rather than keeping a record? change - just send client the info
        // again. their problem.
        else {
            // start connection
            // addRequest(partnerUser, connection->client->user,
            // connection->client->addr.sin_addr,
            // connection->client->addr.sin_port);
            // get the requested user's IP address and port and send
            uint32_t ipaddr; // is stored in network byte order already?
            unsigned short
                port; // is stored in network byte order already? htons
            getUserAddress(clientsList, partnerUser, &ipaddr, &port);
            // this will only work if buffer is empty, which it will be always
            // when this is called

            if (port == 0) { // server doesn't know the requestee's P2P port
                appendBuffer(&connection->sendBuffer,
                             "Cannot connection, user ", 0);
                appendBuffer(&connection->sendBuffer, partnerName, 0);
                appendBuffer(&connection->sendBuffer,
                             "has not provided P2P connection information\n",
                             0);
            } else {
                char *str; // not a string
                // unsigned char ADDRSTRSIZE = 22; // obv this should be
                // #defined if I keep it char str[ADDRSTRSIZE]; // (up to) 15
                // bytes for dotted decimal IP. 1 for space. (up to) 5 bytes for
                // port. null byte.
                unsigned int size =
                    addrToProtocol(ipaddr, port, partnerName,
                                   getUsername(connection->client->user), &str);
                // hot mess
                // // but he already knows the size because had to declare the
                // array, so this is stupid
                appendBuffer(&connection->sendBuffer, str, size);
                setBufferCommand(&connection->sendBuffer, P2P);
                freeProtocolStr(str);
                // this won't work. What if the ipaddr or port has a null byte
                // in it? I'm committed to making my protocol mostly text with a
                // random dodg bit of binary I'll have to write another
                // appendBuffer for the addresses and the client will parse it
                // differently based on the fact that the last byte (command
                // byte) is P2P but how will it know that it is the last byte?
                // because any null byte could either be part of an address or
                // could be end of message how would it tell the difference
                // without reading further content? effectively reading to the
                // next byte and seeing whats there this is stupid or I could
                // double null terminate but that defeats the purpose of
                // terminating my protocol with a null byte, for easy printing
                // etc. I think maybe I should make the protocol all text after
                // all I need null bytes to split messages in the event that
                // multiple are received at once And I'll have to do too much
                // weird stuff to deal with the possibility that the ipaddr or
                // port could contain a null byte. but I hate this text protocol
                // yuck. actually I think maybe I'll just change to end messages
                // with null termination Messages from client to server are this
                // format so I guess it makes sense to have server to client be
                // the same

                // there is another problem, this needs to set the command byte
                // but that is done when sending the buffer, and the
                // handleClient() loop is supposed to send the buffer. I guess I
                // could return the command byte somehow. but I'm already
                // returning bool to continue or not. I would have to have a
                // value to exit, a value to continue and all other values are
                // command bytes to be added. or I could change the sendBuffer
                // implementation to put the command byte at the beginning, and
                // it could be added anytime the sendBuffer struct would have
                // another value for whether the command byte has been set OR if
                // I'm adding another value, then it could just store the
                // command byte itself, and if it is set, add it when it sends.
                // But if I'm adding it when sending then I could also add it as
                // the first byte. But if I'm doing that then I really should
                // always lock in the first byte as different. So should I place
                // a null byte there when there is no command? This would mean
                // that the message then wouldn't be able to start with a null
                // byte, because then would have a double null byte ending the
                // message. This should be okay because the IP in binary form
                // will only be sent when the command byte is set, ie not 0.
                // Seems like a bad requirement though. Actually that wouldn't
                // be a problem anyway because the first byte will be processed
                // separately and then the message reading will start from the
                // second byte. Otherwise I could have a specific value to
                // indicate 'no command' other than 0. Could be anything I guess
                // since the first byte will always be allocated for it. always
                // this is a lot of work just to avoid calling sendBuffer() here
                // instead. I could: store command byte between single null byte
                // and double null byte, or place command byte at first byte or
                // place directly after double null byte, with no single null
                // byte

                // Here's what I'm doing:
                // the first byte of every server to client message is a
                // command/type byte. 0 means no command. I'm not sure whether
                // double null byte termination is required now? If the command
                // byte is set to P2P, then the next 6 bytes are interpreted as
                // the IP address and port. Else, it's just text
            }
        }
    }
    return true;
}

// sendBuffer must be empty
void sendP2PFail(Connection *connection, char *failedName) {
    setBufferCommand(&connection->sendBuffer, P2PFAIL);
    appendBuffer(&connection->sendBuffer, failedName, 0);
    sendBuffer(connection);
}

bool execSetupP2P(char *argsinput, Connection *connection) {
    // argsinput is pointer to start of port in network order binary form
    // it is 2 bytes, then is the double null byte
    memcpy(&connection->client->p2pserverport, argsinput, sizeof(connection->client->p2pserverport));
    return true;
}

bool execIncorrectCommand(Connection *connection) {
    char *msg =
        "Invalid command\n"
        "Valid commands:\n"
        "message <user> <msg>:      Message specific user\n"
        "broadcast <msg>:           Message all online users\n"
        "whoelse:                   List all other online users\n"
        "whoelsesince <seconds>:    List all other users active in the last n "
        "seconds\n"
        "block <user>:              Block a certain user\n"
        "unblock <user>:            Unblock a previously blocked user\n"
        "startprivate <user>        Start P2P connection with specific user\n"
        "private <user> <msg>       Send message over P2P connection to "
        "specific user\n"
        "stopprivate <user>         End P2P connection with specific user\n"
        "logout:                    Logout\n"
        "\n";
    appendBuffer(&connection->sendBuffer, msg, 0);

    return true;
}

bool execCommand(char *command, char *argsinput, Connection *connection) {
    if (strcmp(command, "message") == 0) {
        return execMessage(argsinput, connection);
    } else if (strcmp(command, "broadcast") == 0) {
        return execBroadcast(argsinput, connection);
    } else if (strcmp(command, "whoelse") == 0) {
        return execWhoelse(connection);
    } else if (strcmp(command, "whoelsesince") == 0) {
        return execWhoelsesince(argsinput, connection);
    } else if (strcmp(command, "block") == 0) {
        return execBlock(argsinput, connection);
    } else if (strcmp(command, "unblock") == 0) {
        return execUnblock(argsinput, connection);
    } else if (strcmp(command, "logout") == 0) {
        return false; // why does this return false and stopprivate true
    } else if (strcmp(command, "startprivate") == 0) {
        return execStartPrivate(argsinput, connection);
    // } else if (strcmp(command, "private") == 0) {
    //     return execPrivate(argsinput, connection);
    // } else if (strcmp(command, "stopprivate") == 0) { // this command shouldn't be sent to server
    //     return true;
    } else if (strcmp(command, "setupp2p") == 0) {
        return execSetupP2P(argsinput, connection);
    } else {
        return execIncorrectCommand(connection);
    }
}

// is this like some kind of design pattern thing? reminds me of 2511...
// returns a function that takes a char* and returns a char
/*
bool (*matchCommand(char *command))(char *argsinput, Connection *connection) {
    if (strcmp(command, "message") == 0) {
        return execMessage;
    } else if (strcmp(command, "broadcast") == 0) {
        return execBroadcast;
    } else if (strcmp(command, "whoelse") == 0) {
        return execWhoelse;
    } else if (strcmp(command, "whoelsesince") == 0) {
        return execWhoelsesince;
    } else if (strcmp(command, "block") == 0) {
        return execBlock;
    } else if (strcmp(command, "unblock") == 0) {
        return execUnblock;
    } else if (strcmp(command, "logout") == 0) {
        return execLogout;
    } else if (strcmp(command, "startprivate") == 0) {
        return execStartPrivate;
    } else if (strcmp(command, "private") == 0) {
        return execPrivate;
    } else if (strcmp(command, "stopprivate") == 0) {
        return execStopPrivate;
    } else if (strcmp(command, "setupp2p") == 0) {
        return execSetupP2P;
    } else {
        return execIncorrectCommand;
    }
}
*/

// could affect messages online/offline - another thread determines that user is
// online, sends an online message but then user logs out before it arrives and
// it doesn't get sent as offline message I guess would have to not allow logout
// until all online messages have arrived? But what if other people just keep
// sending messages so can't log out? and also whoelse messages, but that might
// not be a problem OR could implement offline and online messages as the same
// thing and the client just retrieves them whenever ready - when online,
// continuously
// bool execLogout() {
//     /*
//     // no args
//     appendBuffer(&connection->sendBuffer, "Logout\n");
//     logout(connection->user);
//     // notify others of logout - broadcast
//     // more

//     char str[MAXLENGTH]; // name !!
//     snprintf(str, MAXLENGTH, "%s HAS LOGGED OUT",
//     getUsername(connection->user)); broadcast(connection->user, str, SERVER);
//      */

//     return FALSE;
// }

/*
bool execPrivate(char *argsinput, Connection *connection) {
    // char str[MAXLENGTH];
    // snprintf(str, MAXLENGTH, "Something has gone wrong. This command should
    // not have been sent to the server.\n");
    // appendBuffer(&connection->sendBuffer, str, 0);
    return TRUE;
}
*/

/*
bool execStopPrivate(char *argsinput, Connection *connection) {
    // unsigned char numArgs = 2;
    // char *args[numArgs];
    // parseRequest(argsinput, numArgs, args);
    // char *partner = args[0];

    // maybe: this message does not come directly typed in from user but sent
    // from client so the server can keep track of connections? Salil says in
    // forum that shouldn't do this

    // appendBuffer(&connection->sendBuffer, "Something has gone wrong. This
    // command should not have been sent to the server.\n", 0);

    return TRUE;
}
*/
