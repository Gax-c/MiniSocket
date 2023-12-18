#ifndef _INCLUDE_H_
#define _INCLUDE_H_ 

#include <iostream>
#include <cstdio> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <sys/un.h> 
#include <arpa/inet.h> 
#include <netdb.h> 
#include <string> 
#include <pthread.h>
#include <sys/msg.h>
#include <unistd.h>
#include <vector> 
#include <mutex>

#define PORT 3266 
#define MAXLEN 256 
#define QUEUE_LENGTH 25 

enum request_type{CONNECT = 1, REMOVE_CONNECT, GET_TIME, GET_NAME, GET_LIST, SEND_MSG, RECEIVE_MSG}; 

char filepath[] = "include.h"; 

struct packet{ 
    long type; 
    char data[MAXLEN];
}; 

#endif 