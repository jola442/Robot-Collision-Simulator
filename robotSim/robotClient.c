#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "simulator.h"


//Strings of command codes sent from client to server
#define REGISTER_STRING 			"REGISTER"
#define STOP_STRING     			"STOP"
#define CHECK_COLLISION_STRING    	"CHECK COLLISION"
#define STATUS_UPDATE_STRING  		"STATUS UPDATE"


//Strings of command codes sent from server to client
#define OK_STRING          			"OK"
#define NOT_OK_STRING      			"NOT OK"
#define NOT_OK_BOUNDARY_STRING      "NOT OK BOUNDARY"
#define NOT_OK_COLLIDE_STRING       "NOT OK COLLIDE"
#define LOST_CONTACT_STRING  		"LOST CONTACT"


//This function takes in a command code and returns its string representation
char* getBufferString(char buffer){
	if(buffer == OK) return OK_STRING;
	else if(buffer == NOT_OK) return NOT_OK_STRING;
	else if(buffer == NOT_OK_BOUNDARY) return NOT_OK_BOUNDARY_STRING;
	else if(buffer == NOT_OK_COLLIDE) return NOT_OK_COLLIDE_STRING;
	else if(buffer == LOST_CONTACT) return LOST_CONTACT_STRING;
}

//This function takes in a float and returns its first digit
char getFirstDigit(float n){
	while(n >= 10){
		n /= 10;
	}
	return n;
}

//This function takes in a float and returns its last two digits
char getLastTwoDigits(float n){
	int num = (int)n;
	num = num % 100;
	return num;
}

//This function takes in an integer and adjusts it so that it is the range -180 to 180
int adjustDirection(int *d){
	if(*d > 180){
		*d -= 360;
	}
	
	else if(*d < -180){
		*d += 360;
	}
	
}


// This is the main function that simulates the "life" of the robot
// The code will exit whenever the robot fails to communicate with the server
int main() {

	// Set up the random seed
	srand(time(NULL));

	// Register with the server
	int clientSocket, addrSize, bytesReceived, robotBytesReceived;
	struct sockaddr_in serverAddr;		
	unsigned char command[8];			//char array containing request that will be sent to the server
	char* commandString;				//string representation of the command code sent to the server
	unsigned char buffer[8];			//char array containing the responses from the server
	char* bufferString;					//string representing the request received
	char id = 0;						//position of robot in the server's robots array
	Robot robot;						//robot struct


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
	addrSize = sizeof(serverAddr);


	// Send command string to server
	command[0] = REGISTER;
	commandString = REGISTER_STRING;
	sendto(clientSocket, command, sizeof(command), 0,(struct sockaddr *) &serverAddr, addrSize);

	printf("CLIENT: Sending  \"%s\" to server.\n", commandString);

	bytesReceived = recvfrom(clientSocket, &buffer, sizeof(buffer), 0,(struct sockaddr *) &serverAddr, &addrSize);
	bufferString = getBufferString(buffer[0]);
	printf("CLIENT: Got back command \"%s\" from server.\n", bufferString);

	// Send register command to server.  Get back command data
	if(bytesReceived > 0){
		id = buffer[1];
		command[1] = id;
		robot.x = (buffer[2] * 100) + buffer[3];
		robot.y = (buffer[4] * 100) + buffer[5];
		robot.direction = (buffer[7] == 1)? buffer[6] * -1 : buffer[6];	

	// and store it.   If denied registration, then quit.
		if(buffer[0] == NOT_OK){
			close(clientSocket);
			printf("CLIENT: Shutting down...\n");
			return 0;
		}

	}

	char generatedRandomTurn = 0;						//boolean representing whether a random turn has been generated or not
	int leftOrRight = 0;								//boolean representing whether the robot turns left or right
	char online = 1;									
	// Go into an infinite loop exhibiting the robot behavior
	while (online) {
		addrSize = sizeof(serverAddr);
		// Check if can move forward
		command[0] = CHECK_COLLISION;												//command code
		command[1] = id;															//ID of robot
		command[2] = (robot.x < 100)? 0 : getFirstDigit(robot.x);					//first digit of robot's x
		command[3] = getLastTwoDigits(robot.x);									    //last two digits of robot's x
		command[4] = (robot.y < 100)? 0 : getFirstDigit(robot.y);					//first digit of robot's y
		command[5] = getLastTwoDigits(robot.y);										//last two digits of robot's y
		command[6] = abs(robot.direction);											//robot's direction
		command[7] = (robot.direction < 0)? 1 : 0;     							    //sign byte of direction
		
		
		sendto(clientSocket, command, sizeof(command), 0, (struct sockaddr *) &serverAddr, addrSize);
/*		printf("CLIENT: Sending  \"%s\" to server.\n", commandString);*/

		// Get command from server.
		bytesReceived = recvfrom(clientSocket, &buffer, sizeof(buffer), 0,(struct sockaddr *) &serverAddr, &addrSize);
		
		//If the client received bytes from the server
		if(bytesReceived > 0){		
			// If ok, move forward
			if(buffer[0] == OK){
				generatedRandomTurn = 0;
				float x, y, d;
				x = robot.x;
				y = robot.y;
				d = robot.direction * PI/180;

				robot.x += ROBOT_SPEED * cos(d);
				robot.y += ROBOT_SPEED * sin(d);
							
			}
			
			// Otherwise, we could not move forward, so make a turn.
			else if(buffer[0] == NOT_OK_COLLIDE || buffer[0] == NOT_OK_BOUNDARY){
				if(!generatedRandomTurn){
					leftOrRight = rand() % 2;
					if(leftOrRight == 1){
						robot.direction = robot.direction + ROBOT_TURN_ANGLE;		//turn the robot left
						adjustDirection(&(robot.direction));
					
					}
					
					else if(leftOrRight == 0){
						robot.direction = robot.direction - ROBOT_TURN_ANGLE;		//turn the robot right
						adjustDirection(&(robot.direction));
					}	
					generatedRandomTurn = 1;	
				}
	
				// If we were turning from the last time we collided, keep
				// turning in the same direction, otherwise choose a random 
				// direction to start turning.
				else{
					if(leftOrRight == 1){
						robot.direction = robot.direction + ROBOT_TURN_ANGLE;		//turn the robot right
						adjustDirection(&(robot.direction));
					}
					
					else if(leftOrRight == 0){
						robot.direction = robot.direction - ROBOT_TURN_ANGLE;		//turn the robot left
						adjustDirection(&(robot.direction));
					}		
				}
			}
			
			//If the robot has lost contact to the server, stop sending requests.
			else if(buffer[0] == LOST_CONTACT){
				break;
			}
			
		}
			


		   
		// Send update to server
		command[0] = STATUS_UPDATE;													//command code
		command[1] = id;															//ID of robot
		command[2] = (robot.x < 100)? 0 : getFirstDigit(robot.x);					//high byte of robot's x
		command[3] = getLastTwoDigits(robot.x);									    //low byte of robot's x
		command[4] = (robot.y < 100)? 0 : getFirstDigit(robot.y);					//high byte of robot's y
		command[5] = getLastTwoDigits(robot.y);										//low byte of robot's y
		command[6] = abs(robot.direction);											//robot's direction
		command[7] = (robot.direction < 0)? 1 : 0;     							    //sign byte of direction
		sendto(clientSocket, command, sizeof(command), 0, (struct sockaddr *) &serverAddr, addrSize);
		

		// Uncomment line below to slow things down a bit 
		usleep(10000);
	}
	close(clientSocket); 
	printf("CLIENT: Shutting down.\n");
	
	
}

