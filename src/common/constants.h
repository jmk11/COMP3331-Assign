#ifndef PROJECT_CONSTANTS_H
#define PROJECT_CONSTANTS_H

#define MAXPORT 65535 // !! get max short value #defined somewhere in C - MAXSHORT?
#define MAXATTEMPTS 3
#define MAXQUEUE 5 // !!! some of these should just be with server?

// ERRORS:
// 1 and 2 are standard error numbers
#define ARGSERROR 3
#define CREDENTIALSERROR 4
#define BUILDSOCKETERROR 5
#define CLOSESOCKETERROR 6
#define CLOCKERROR 7
#define MALLOCERROR 8
#define THREADERROR 9
#define SEMAPHOREERROR 10
#define SENDERROR 11
#define EVENTFDERROR 12
#define SIGHANDLERERROR 11
#define TIMERFDERROR 14
#define FILERROR 15

#define SIGINTEXIT 20 // remove?
// enum??

// LOGIN:
#define LOGINSUCCESSFUL 0
#define INVALIDUSERNAME 1
#define INCORRECTPASSWORD 2
#define BARRED 3
#define NOWBARRED 4
#define LOGGEDIN 5

//#define NOINPUT 0
//#define INPUTREQUIRED 1

#define MAXLENGTH 150 // name !!

//#define MAXNUMARGS 2

//#define TRUE 1
//#define FALSE 0
//typedef char bool;

typedef char byte;

#define CLKID CLOCK_BOOTTIME

// REMOVE THIS !!
//typedef struct in_addr IPType;

// !! I think this should not be in this file??
enum MessageType { USER,
                   SERVER }; // names - info notification?
typedef enum MessageType MessageType;

#endif //PROJECT_CONSTANTS_H

// this #ifndef stuff was automatically added by clion. idk what it is
