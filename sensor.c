#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/msg.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "msgProtocol.h"
#include <stdbool.h>

int main(int argc, char* argv[]){


	dev_data_t send_data_1;

	ack_data_t received_data_from_ctrl;

	char str_buffer [128];
	bool Sensor_running = 1;
	char devName [16];
	int randNum, threshold,msg_recv_status,upper_limit;
	
	int msg_q_ID;
	long int curr_PID = (long int) getpid();
	long int recv_msg_type = curr_PID*10;
	
	
	printf("The current PID is: %li\n-------------------------\n", curr_PID );
	send_data_1.msg_type =  curr_PID;
	printf("The Message Type is: %li\n", send_data_1.msg_type );

	//Catches if less than 2 arguments are supplied
	if(argc<2){
		fprintf(stderr, "ERROR: Not enough arguments supplied\n\tRequired Parameters for Sensor: Device Name, Threshold Value\n\n");
        	exit(EXIT_FAILURE);
	}

	strcpy(devName,"SEN: ");
	strcat(devName,argv[1]);

	//Creates a message queue with a key
	msg_q_ID = msgget((key_t)MSG_Q_KEY, 0666);
	
	//If message queue creation fails
 	if (msg_q_ID == -1) {
        	fprintf(stderr, "ERROR: Message Queue creation failed. Err #%d\n",errno);
        	exit(EXIT_FAILURE);
    	}
	printf("Message Queue Used: %d\n",msg_q_ID);


	

	threshold = atoi(argv[2]);

	upper_limit = threshold*5/4;

	//Structuring initial message
	send_data_1.msg_data.pid =curr_PID;
	send_data_1.msg_data.status = STATUS_INIT;
	send_data_1.msg_data.trshVal = threshold;
	send_data_1.msg_data.currVal = 0;
	strcpy(send_data_1.msg_data.devType,devName);



	printf("Sending Initial message -->  PID: %d\tDevice Status: %d\tThreshold Value : %d\n", 
			send_data_1.msg_data.pid,send_data_1.msg_data.status,send_data_1.msg_data.trshVal);

	if(msgsnd(msg_q_ID, (void *)&send_data_1, BUFFER_SIZE, 0) == -1){
		fprintf(stderr, "ERROR: Message not sent to Controller. Err # %d\n", errno);
		exit(EXIT_FAILURE);
	}

	if (msgrcv(msg_q_ID, (void*)&received_data_from_ctrl,BUFFER_SIZE, (long int) recv_msg_type , 0 ) == -1) {
		fprintf(stderr, "ERROR: Message not received from Controller. Err # %d\n", errno);
		exit(EXIT_FAILURE);
	}

	sleep(2);
	printf("Message from Controller: %s\n------------------------\n",received_data_from_ctrl.ack_msg);
	if( strncmp(received_data_from_ctrl.ack_msg, "ACK", 3) == -1){
		fprintf(stderr, "ERROR: Did not receive ACK Message. Err #%d\n",errno);
        exit(EXIT_FAILURE);
	}



	send_data_1.msg_data.status = STATUS_NORMAL;


	printf("Running Normal Behaviour\n------------------------\n");
	sleep(2);
	//Periodically sending a random number every second
	while(Sensor_running){
		printf("Running Normal Behaviour\n------------------------\n");

		if (msgrcv(msg_q_ID, (void*)&received_data_from_ctrl,BUFFER_SIZE, (long int) recv_msg_type , IPC_NOWAIT ) != -1) {
			if(strncmp(received_data_from_ctrl.ack_msg,"end",3)==0){
				Sensor_running = false;
				break;
			}
		printf("Inside message Receive\n");

		}
		printf("Check 1\n");
		randNum = rand()%(upper_limit);
		printf("Check 2\n");
		send_data_1.msg_data.currVal = randNum;
		printf("Sending --> Message PID: %d\tDevice Status: %d\tThreshold Value : %d\tCurrent Value: %d\n", 
				send_data_1.msg_data.pid,send_data_1.msg_data.status,send_data_1.msg_data.trshVal,send_data_1.msg_data.currVal);
		if(msgsnd(msg_q_ID, (void *)&send_data_1, BUFFER_SIZE, 0) == -1){
			fprintf(stderr, "ERROR: Message not sent to Controller. Err # %d\n", errno);
			exit(EXIT_FAILURE);
		}
		sleep(1);

	}
}
