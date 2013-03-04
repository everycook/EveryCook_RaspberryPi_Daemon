#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "bool.h"
#include "convertFunctions.h"
#include "middlewareSocket.h"

void error(const char *msg){
    perror(msg);
}

void middlewareEndlessTestLoop(char* hostname, int portno){	
	int sockfd = connectToMiddleware(hostname, portno);
	
	uint8_t dataType = TYPE_TEXT;
	
	//run logic
	char buffer[256];
	bzero(buffer,256);
	uint8_t type = 0;
	while(1==1){
		printf("Please enter the message: ");
		bzero(buffer,256);
		fgets(buffer,255,stdin);
		if (!sendToSock(sockfd, buffer, dataType)){
			error("ERROR writing to socket");
		}
		
		if (isSocketReady(sockfd)){
			if (recivePackageFromSock(sockfd, buffer, &type)){
				printf("%s, type: %d\n", buffer, type);
			} else {
				error("ERROR reading from socket");
			}
		}
	}
}

int connectToMiddleware(char* hostname, int portno) {
	int sockfd;
	struct hostent *server;
	
	server = gethostbyname(hostname);
	if (server == NULL){
		error("ERROR, no such host\n");
		return -11;
	}
	
	sockfd = connectToServer(server, portno);
	
	if (sockfd < 0){
		if (sockfd == -1){
			error("ERROR opening socket");
		} else if (sockfd == -2){
			error("ERROR connecting");
		}
		return sockfd;
	}
	if (doHandshake(sockfd, "everycook")){
		return sockfd;
	} else {
		close(sockfd);
		return -12;
	}
}

int connectToServer(struct hostent *server, int portno){
	int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
		return -1;
	}
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
		return -2;
	}
	return sockfd;
}

bool doHandshake(int sockfd, char* application){
	int n;
	char sendBuffer[256];
	bzero(sendBuffer,256);
	StringUnion(sendBuffer, "ServerConnect\r\n");
	StringUnion(sendBuffer, "application: ");
	StringUnion(sendBuffer, application);
	StringUnion(sendBuffer, "\r\n");
	//add other "headers/parameters if needed ("<param>: <value>")
	
	n = write(sockfd,sendBuffer,strlen(sendBuffer));
	if (n < 0) {
		return false;
	}
	return true;
}


bool sendToSock(int sockfd, char* buffer, uint8_t type){
	int n;
	uint16_t len = strlen(buffer);
	//char sendBuffer[4];
	char sendBuffer[259];
	bzero(sendBuffer,259);
	sendBuffer[0] = len >> 8 & 0x00FF;
	sendBuffer[1] = len & 0x00FF;
	sendBuffer[2] = type;
	sendBuffer[3] = 0x00;
	int i;
	for (i=0; i<len; ++i){
		sendBuffer[i+3] = buffer[i];
	}
	n = write(sockfd,sendBuffer,len+3);
	/*
	n = write(sockfd,sendBuffer,3);
	if (n < 0) {
		return -1;
	}
	n = write(sockfd,buffer,len);
	*/
	if (n < 0) {
		return false;
	}
	return true;
}

bool isSocketReady(int sockfd){
	int 	res;
	fd_set	sready;
	struct timeval	nowait;
	FD_ZERO(&sready);
	FD_SET((unsigned int)sockfd, &sready);
	memset((char *)&nowait,0,sizeof(nowait));
	
	res = select(sockfd+1, &sready, NULL, NULL, &nowait);
	if (FD_ISSET(sockfd, &sready)){
		res = true;
	} else {
		res = false;
	}
	return res;
}
/*
bool reciveFromSock(int sockfd, char* buffer){
	int n;
	bzero(buffer,256);
	n = read(sockfd,buffer,255);
	if (n <= 0) {
		return false;
	}
	return true;
}
*/

bool recivePackageFromSock(int sockfd, char* buffer, uint8_t* type){
	int n;
	bzero(buffer,256);
	
	char reciveBuffer[259];
	bzero(reciveBuffer,259);
	n = read(sockfd,reciveBuffer,259);
	if (n <= 0) {
		return false;
	}
	if (n<3){
		return false;
	}
	uint16_t len = (reciveBuffer[0] << 8) + reciveBuffer[1];
	*type = reciveBuffer[2];
	int i;
	for (i=0; i<len; ++i){
		buffer[i] = reciveBuffer[i+3];
	}
	return true;
}