#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"

#define STOP_STRING     			"STOP"
#define LOST_CONTACT_STRING  		"LOST CONTACT"

int main() {
  // ... ADD SOME VARIABLES HERE ... //
	int clientSocket, addrSize, bytesReceived;
	struct sockaddr_in serverAddr;
	char command;
	char* commandString;
	char* bufferString;
	char buffer[8];

  // Register with the server
  
   // Create socket
	clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (clientSocket < 0) {
		printf("*** CLIENT ERROR: Could open socket.\n");
		exit(-1);
	}
	
	// Setup address
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);

  
  // Send command string to server
  	addrSize = sizeof(serverAddr);
	command = STOP;
	commandString = STOP_STRING;
	printf("CLIENT: Sending  \"%s\" to server.\n", commandString);
	sendto(clientSocket, &command, sizeof(command), 0,(struct sockaddr *) &serverAddr, addrSize);
	bytesReceived = recvfrom(clientSocket, &buffer, sizeof(buffer), 0,(struct sockaddr *) &serverAddr, &addrSize);
	
	if(buffer[0] == LOST_CONTACT) bufferString = LOST_CONTACT_STRING;

	printf("CLIENT: Got back response \"%s\" from server.\n", bufferString);

	close(clientSocket);
	printf("CLIENT: Shutting down...\n");


}
