#include <netdb.h> 
#include "bool.h"
void error(const char *msg);
int connectToMiddleware(char* hostname, int portno);
int connectToServer(struct hostent *server, int portno);
bool doHandshake(int sockfd, char* application);
bool sendToSock(int sockfd, char* buffer, uint8_t type);
bool isSocketReady(int sockfd);
/*bool reciveFromSock(int sockfd, char* buffer);*/
bool recivePackageFromSock(int sockfd, char* buffer, uint8_t* type);

/*
#ifndef MIDDLEWARE_TYPES
#define MIDDLEWARE_TYPES
const uint8_t TYPE_TEXT         = 1;
const uint8_t TYPE_BINARY       = 2;
const uint8_t TYPE_CLOSE        = 8;
#endif
*/

#define TYPE_TEXT 		1;
#define TYPE_BINARY 	2;
#define TYPE_CLOSE 		8;