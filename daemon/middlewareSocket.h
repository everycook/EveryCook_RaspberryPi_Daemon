/*
This is the EveryCook Raspberry Pi daemon. It reads inputs from the EveryCook Raspberry Pi shield and controls the outputs.
EveryCook is an open source platform for collecting all data about food and make it available to all kinds of cooking devices.

This program is copyright (C) by EveryCook. Written by Samuel Werder, Peter Turczak and Alexis Wiasmitinow.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

See GPLv3.htm in the main folder for details.
*/

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