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
#include <signal.h>

typedef struct reg_sen_struct {
	long int msg_type;
	pid_t pid;
	int trshVal;
	char devType[16];
} reg_sen;

typedef struct reg_act_struct {
	long int msg_type;
	pid_t pid;
	char devType[16];
} reg_act;


static int Message_Queue_ID;

void sig_handler(int signo)
{	
	dev_data_t catch_data;
	long int msg_type;
	pid_t curr_PID = getpid();
    if (signo == SIGUSR1){
    	printf("SIGUSR1 Received\n");
    	//Get a message from the Message Queue
		msg_type = msgrcv(Message_Queue_ID, (void*)&catch_data, BUFFER_SIZE,curr_PID, 0 );
		if (msg_type == -1) {
					fprintf(stderr, "Message receive from Message Queu failed with error: %d\n", errno);
					exit(EXIT_FAILURE);
		}
		else if(msg_type = curr_PID){
			printf("Data Received: PID: %d\n",catch_data.msg_data.pid);
		}
	}
}




int main(int argc, char* argv[]){
	pid_t currentPID;
	dev_data_t received_data;

	currentPID = fork();


	if(currentPID >= 0) // fork was successful
	{
		if(currentPID == 0) // child process
		{
			static int msg_q_ID;
			bool running = true;
			int msg_recv_status;
			char dev_count_sen= 0;;
			char dev_count_act = 0;
;
			int i;
			int sig = 1;
			pid_t parent_PID;

			static ack_data_t cntrl_to_dev;
			static reg_sen registered_sen[3];
			static reg_act registered_act[3];


			printf("\n Child Process: %d\n", getpid() );
			parent_PID = getppid();
			
			msg_q_ID = msgget((key_t)MSG_Q_KEY, 0666 | IPC_CREAT);

			Message_Queue_ID =  msg_q_ID;
			printf("Message Q ID:  %d\n",msg_q_ID);	
			sleep(5);

			while(running){

				//Get a message from the Message Queue
				msg_recv_status = msgrcv(msg_q_ID, (void*)&received_data, BUFFER_SIZE, 0, 0 );
				
				if (msg_recv_status == -1) {
					fprintf(stderr, "Message receive from Message Queu failed with error: %d\n", errno);
					exit(EXIT_FAILURE);
				}

				//Check to see if a sensor or actuator is sending a message
				if (received_data.msg_type < 10000) {
					printf("Device status: %d\n",received_data.msg_data.status);

					//If the check a device needed initialization
					if (received_data.msg_data.status == STATUS_INIT) {
						printf("Initializing Device in Database\n");
				   			
						//Sets up Acknowledge Data to be sent
						cntrl_to_dev.msg_type = (long int)received_data.msg_type * 10;
						strcpy(cntrl_to_dev.ack_msg,"ACK");
						
						printf("Sent: %li  %s\n",cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);

						//Send Acknowledgement data
						if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, BUFFER_SIZE, 0) == -1) {
				    		fprintf(stderr, "ERROR: Failed to send Ackowledge to Device. Error # %d \n",errno);
				   			exit(EXIT_FAILURE);
						}
						
						//If a sensor needs initializing
						if( strncmp(received_data.msg_data.devType, "SEN", 3) == 0) {
							//Records Device Data
							registered_sen[dev_count_sen].msg_type = received_data.msg_type;
							registered_sen[dev_count_sen].pid = received_data.msg_data.pid;
							strcpy(registered_sen[dev_count_sen].devType,received_data.msg_data.devType);
							registered_sen[dev_count_sen].trshVal = received_data.msg_data.trshVal;		
							dev_count_sen++;
						}	
						else if( strncmp(received_data.msg_data.devType, "ACT", 3) == 0){
							//Records Device Data
							registered_act[dev_count_act].msg_type = received_data.msg_type;
							registered_act[dev_count_act].pid = received_data.msg_data.pid;
							strcpy(registered_act[dev_count_act].devType,received_data.msg_data.devType);
							dev_count_act++;
						}
				   		
							
					}

					else if (received_data.msg_data.status == STATUS_NORMAL) {
						printf("Received --> Device Name: %d\tThreshold Value : %d\tCurrent Value: %d\n",
										received_data.msg_data.pid,received_data.msg_data.trshVal,received_data.msg_data.currVal);
								
						for (i=0; i <2; i++){

							if(registered_sen[i].pid==received_data.msg_data.pid){
								printf("Received --> Device Name: %s\tThreshold Value : %d\tCurrent Value: %d\n",
										received_data.msg_data.devType,received_data.msg_data.trshVal,received_data.msg_data.currVal);
								if(received_data.msg_data.currVal >= registered_sen[i].trshVal){
									printf("OH NO!!!!!!\n");
									//running = false;
									//Sets up Acknowledge Data to be sent
									cntrl_to_dev.msg_type = registered_act[i].msg_type * 10;
									strcpy(cntrl_to_dev.ack_msg,"Activate");
									//Send Acknowledgement data
									printf("Sending To ACT--> Message Type: %li\tDevice message: %s\n", cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);
									if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, sizeof(ack_data_t), 0) == -1) {
							    		fprintf(stderr, "Message send to controller failed with Error no: %d \n",errno);
							   			exit(EXIT_FAILURE);
									}
									received_data.msg_type = parent_PID;
									kill(parent_PID, SIGUSR1);	
									if (msgsnd(msg_q_ID, (void *)&received_data, sizeof(received_data), 0) == -1) {
							    		fprintf(stderr, "Message send to Parent Process failed with Error no: %d \n",errno);
							   			exit(EXIT_FAILURE);
									}
								
								}
								break;	
								sleep(1);		
							}	
						}
					}
				}
				//If the message type is sent by the controller
				else{
					//Send the message back to the message queue
					msgsnd(msg_q_ID, (void*)&received_data, BUFFER_SIZE, 0);
				}
			}
		
		}
		else //Parent process
		{
			if (signal(SIGUSR1, sig_handler) == SIG_ERR)
        		printf("\ncan't catch SIGUSR1\n");
			}

			wait(1);
		}
	else // fork failed
	{
		printf("\n Fork failed, quitting!!!!!!\n");
		return 1;
	}

	
	printf("Exit program\n");
	return 0;

}
