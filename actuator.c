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

	static int msg_q_ID;
	static bool actuator_running;
	static char devName [16];
	static int randNum, threshold ,msg_recv_status;
	static long int curr_PID;
	static long int recv_msg_type;
	static char str_buffer [128];
	static dev_data_t send_data_1;
	static ack_data_t received_data_from_ctrl;


	actuator_running = true;
	curr_PID = getpid();
	recv_msg_type = curr_PID*10;


	printf("The current PID is: %li\n-------------------------\n", curr_PID );
	send_data_1.msg_type =  curr_PID;
	printf("The Message Type is: %li\n", send_data_1.msg_type );
	
	//Catches if less than 2 arguments are supplied
	if(argc<1){
		fprintf(stderr, "ERROR: Not enough arguments supplied\n\tRequired Parameters for Actuator: Device Name \n\n");
        	exit(EXIT_FAILURE);
	}

	strcpy(devName,"ACT: ");
	strcat(devName,argv[1]);

	//Creates a message queue with a key
	msg_q_ID = msgget((key_t)MSG_Q_KEY, 0666);
	
	//If message queue creation fails
 	if (msg_q_ID == -1) {
        	fprintf(stderr, "ERROR: Message Queue creation failed. Err #%d\n",errno);
        	exit(EXIT_FAILURE);
    	}
	printf("Message Queue Used: %d\n",msg_q_ID);


	//Structuring initial message
	send_data_1.msg_data.pid = curr_PID;
	send_data_1.msg_data.status = STATUS_INIT;
	send_data_1.msg_data.trshVal = 0;
	send_data_1.msg_data.currVal = 0;
	strcpy(send_data_1.msg_data.devType,devName);

	printf("Sending Initial message -->  PID: %d\tDevice name: %s\tDevice Status: %d\n", 
			send_data_1.msg_data.pid,send_data_1.msg_data.devType,send_data_1.msg_data.status);
	
	if(msgsnd(msg_q_ID, (void *)&send_data_1, BUFFER_SIZE, 0) == -1){
		fprintf(stderr, "ERROR: Message not sent to Controller. Err # %d\n", errno);
		exit(EXIT_FAILURE);
	}


	if (msgrcv(msg_q_ID, (void*)&received_data_from_ctrl,BUFFER_SIZE, recv_msg_type , 0 ) == -1) {
		fprintf(stderr, "ERROR: Message not received. Err # %d\n", errno);
		exit(EXIT_FAILURE);
	}
	printf("Message from Controller: %s\n------------------------\n",received_data_from_ctrl.ack_msg);
	
	if( strncmp(received_data_from_ctrl.ack_msg, "ACK", 3) == -1){
		fprintf(stderr, "ERROR: Did not receive ACK Message. Err #%d\n",errno);
        exit(EXIT_FAILURE);
	}



	send_data_1.msg_data.status = STATUS_NORMAL;
	printf("Running Normal Behaviour\n------------------------\n");
	//printf("Device Status: %d\n", send_data_1.msg_data.status);

	//Periodically sending a random number every second
	while(actuator_running){

		if (msgrcv(msg_q_ID, (void*)&received_data_from_ctrl,sizeof(ack_data_t), (long int) recv_msg_type , 0 )== -1) {
			fprintf(stderr, "ERROR: Did not receive Message from Controller. Err #%d\n",errno);
       		exit(EXIT_FAILURE);
		}
		if(strncmp(received_data_from_ctrl.ack_msg,"STOP",4)==0){
			actuator_running = false;
			break;
		}
		else{
			printf("Command: %s %s.\n",argv[1],received_data_from_ctrl.ack_msg);
		}

	}
	printf("Exit program\n");
	return 0;
}
