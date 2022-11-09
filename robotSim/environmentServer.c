#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netinet/in.h>

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


Environment    environment;  // The environment that contains all the robots

//This function takes in a command code and returns the string representation
char* getBufferString(char buffer){
	if(buffer == REGISTER) return REGISTER_STRING;
	else if(buffer == STOP) return STOP_STRING;
	else if(buffer == CHECK_COLLISION) return CHECK_COLLISION_STRING;
	else if(buffer == STATUS_UPDATE) return STATUS_UPDATE_STRING;
	return "";
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

//This function takes in an environment pointer, a robot struct and a robot ID representing the position of the robot in the environment's robot lists.
//It returns NOT_OK_BOUNDARY, NOT_OK_COLLIDE and OK depending on whether the robot can move forward or not.
char robotCanMoveForward(Environment *env, Robot robot, char robotID){	
	int i;									//counter variable
	float dx, dy, d;					    //values used to calculate new robot (x,y)
	d = robot.direction * PI/180;			//converting the direction to radians

	//Computing new direction
	robot.x += ROBOT_SPEED * cos(d);
	robot.y += ROBOT_SPEED * sin(d);

	//Boundary checks
	if(robot.x - ROBOT_RADIUS <= 0 || robot.x + ROBOT_RADIUS >= ENV_SIZE){
		return NOT_OK_BOUNDARY;
	}
	
	if(robot.y - ROBOT_RADIUS <= 0 || robot.y + ROBOT_RADIUS >= ENV_SIZE){
		return NOT_OK_BOUNDARY;
	}
	
	//Collision checks
	for(i = 0; i < env->numRobots; i++){
		if(robotID == i) continue;
		
		Robot robotTwo = env->robots[i];
		dx = robot.x - robotTwo.x;
		dy = robot.y - robotTwo.y;
		double distance = sqrt( (dx * dx) + (dy * dy) );
		
		if(distance <= ROBOT_RADIUS*2){
			return NOT_OK_COLLIDE;
		}
	}
	

	return OK;
}




// Handle client requests coming in through the server socket.  This code should run
// indefinitiely.  It should repeatedly grab an incoming messages and process them. 
// The requests that may be handled are STOP, REGISTER, CHECK_COLLISION and STATUS_UPDATE.   
// Upon receiving a STOP request, the server should get ready to shut down but must
// first wait until all robot clients have been informed of the shutdown.   Then it 
// should exit gracefully.  
void *handleIncomingRequests(void *e) {
	int serverSocket;
	struct sockaddr_in serverAddr, clientAddr;
	int status, addrSize, bytesReceived;
	fd_set readfds, writefds;
	unsigned char buffer[8];				//char array representing the request received from the client
	char online = 1;						//boolean representing whether the server has been shut down or not
	unsigned char response[8];				//char array representing the response sent to the client
	char* bufferString;						//string representation of the request received from the client
	char *responseString;					//string representation of response code sent to the client
	char lostContact = 0;					//boolean representing whether a STOP request has been sent by the stop.c program or not
	
	Environment *env = ((Environment *)e);		

	
	// Create the server socket
	serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket < 0) {
		printf("*** SERVER ERROR: Could not open socket.\n");
		exit(-1);
	}
	
	// Setup the server address
	memset(&serverAddr, 0, sizeof(serverAddr)); // zeros the struct
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons((unsigned short) SERVER_PORT);
	
	// Bind the server socket
	status = bind(serverSocket,(struct sockaddr *)&serverAddr, sizeof(serverAddr));

	if (status < 0) {
		printf("*** SERVER ERROR: Could not bind socket.\n");
		exit(-1);
	}

	
	int numShutDown = 0;
  	// Wait for clients now
	while (online) {
		FD_ZERO(&readfds);
		FD_SET(serverSocket, &readfds);
		FD_ZERO(&writefds);
		FD_SET(serverSocket, &writefds);
		status = select(FD_SETSIZE, &readfds, &writefds, NULL, NULL);
		if (status == 0) { // Timeout occurred, no client ready
			
		}
		
		else if (status < 0) {
			printf("*** SERVER ERROR: Could not select socket.\n");
			exit(-1);
		}
		else {
			addrSize = sizeof(clientAddr);
			bytesReceived = recvfrom(serverSocket, &buffer, sizeof(buffer), 0, (struct sockaddr *) &clientAddr, &addrSize);
			//If the server received a request from the client
			if (bytesReceived > 0) {
/*				bufferString = getBufferString(buffer[0]);*/
/*				printf("SERVER: Received client request: \"%s\"\n", bufferString);*/
				if(lostContact){
					response[0] = LOST_CONTACT;
					responseString = LOST_CONTACT_STRING;
					sendto(serverSocket, &response[0], 1, 0, (struct sockaddr *) &clientAddr, addrSize);
					numShutDown++;
					printf("Num robots:%d\n",env->numRobots);
					printf("Num robots shut down:%d\n",numShutDown);
/*					printf("SERVER: Sending \"%s\" to client\n", responseString);*/
				}
				
				
				//If a register request was received
				else if(buffer[0] == REGISTER){
					if(env->numRobots < MAX_ROBOTS){
						env->robots[env->numRobots].x = ( rand() % ( (ENV_SIZE - ROBOT_RADIUS)+1 ) ) + ROBOT_RADIUS;
						env->robots[env->numRobots].y = ( rand() % ( (ENV_SIZE - ROBOT_RADIUS)+1 ) ) + ROBOT_RADIUS;
						env->robots[env->numRobots].direction = ( rand()% 361 ) - 180;
						
						//Continue generating random values until the robot does not spawn on any other robots
						while(robotCanMoveForward(env, env->robots[env->numRobots], env->numRobots) != OK){
							env->robots[env->numRobots].x = ( rand() % ( (ENV_SIZE - ROBOT_RADIUS)+1 ) ) + ROBOT_RADIUS;
							env->robots[env->numRobots].y = ( rand() % ( (ENV_SIZE - ROBOT_RADIUS)+1 ) ) + ROBOT_RADIUS;
							env->robots[env->numRobots].direction = ( rand()% 361 ) - 180;
						}

						env->numRobots++;
						
						response[0] = OK;																											//response code
						response[1] = env->numRobots-1;																								//ID of robot
						response[2] = (env->robots[env->numRobots-1].x < 100)? 0 : getFirstDigit(env->robots[env->numRobots-1].x);					//first digit of robot's x
						response[3] = getLastTwoDigits(env->robots[env->numRobots-1].x);															//last two digits of robot's x
						response[4] = (env->robots[env->numRobots-1].y < 100)? 0 : getFirstDigit(env->robots[env->numRobots-1].y);					//first digit of robot's y
						response[5] = getLastTwoDigits(env->robots[env->numRobots-1].y);															//last two digits of robot's y
						response[6] = abs(env->robots[env->numRobots-1].direction);																	//robot's direction
						response[7] = (env->robots[env->numRobots-1].direction < 0) ?  1 : 0;														//sign byte of the robot's direction
			

						
						sendto(serverSocket, response, sizeof(response), 0, (struct sockaddr *) &clientAddr, addrSize);
/*						responseString = OK_STRING;*/
/*						printf("SERVER: Sending \"%s\" to client\n", responseString);*/
					}
					
					//If robot can't be registered
					else{
						response[0] = NOT_OK;
						sendto(serverSocket, response, sizeof(response), 0, (struct sockaddr *) &clientAddr, addrSize);
/*						responseString = NOT_OK_STRING;*/
/*						printf("SERVER: Sending \"%s\" to client\n", responseString);*/
					}
					
				}
				
				//If a check collision request was received
				else if(buffer[0] == CHECK_COLLISION){
					char robotID = buffer[1];
					env->robots[robotID].x = (buffer[2] * 100) + buffer[3];
					env->robots[robotID].y = (buffer[4] * 100) + buffer[5];
					env->robots[robotID].direction = (buffer[7] == 1)? buffer[6] * -1 : buffer[6];	
					
					if(robotCanMoveForward(env, env->robots[robotID], robotID) == NOT_OK_BOUNDARY){
						response[0] = NOT_OK_BOUNDARY;
						responseString = NOT_OK_BOUNDARY_STRING;
					}
					
					else if(robotCanMoveForward(env, env->robots[robotID], robotID) == NOT_OK_COLLIDE){
						response[0] = NOT_OK_COLLIDE;
						responseString = NOT_OK_COLLIDE_STRING;
					}
					
					else if(robotCanMoveForward(env, env->robots[robotID], robotID) == OK){
						response[0] = OK;
						responseString = OK_STRING;
					
					}
					sendto(serverSocket, response, sizeof(response), 0, (struct sockaddr *) &clientAddr, addrSize);
/*					printf("SERVER: Sending \"%s\" to client\n", responseString);*/
					
				}
				
				//If a status update request was received
				else if(buffer[0] == STATUS_UPDATE){
					char robotID = buffer[1];
					env->robots[robotID].x = (buffer[2] * 100) + buffer[3];
					env->robots[robotID].y = (buffer[4] * 100) + buffer[5];
					env->robots[robotID].direction = (buffer[7] == 1)? buffer[6] * -1 : buffer[6];

				}
					
				// If the client said to stop, then I'll stop myself
				else if(buffer[0] == STOP){
					lostContact = 1;
				}
				
				
				//If all robots have shut down, shut down the server
				if(lostContact && numShutDown == env->numRobots){
					online = 0;
				}	

  		}	
	}
	}
	

	environment.shutDown = 1;
	close(serverSocket);

 	printf("SERVER: Shutting down.\n");
 
}




int main() {
	// So far, the environment is NOT shut down
	environment.numRobots = 0;
	environment.shutDown = 0;
  
	// Set up the random seed
	srand(time(NULL));

	
	pthread_t t1, t2;
	
	pthread_create(&t1, NULL, handleIncomingRequests, &environment);
	pthread_create(&t2, NULL, redraw, &environment);

	// Spawn an infinite loop to handle incoming requests and update the display
	while(!environment.shutDown){
		// Wait for the update and draw threads to complete
		pthread_join(t1, NULL);
		pthread_join(t2, NULL);
	}
	
}
