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

static pid_t childPID;

static cloud_st cloud_data;

static volatile bool stop_devices;
static volatile bool controller_running;

static int cloud_fifo_fd, parent_fifo_fd;

void parent_sig_handler(int signo)
{	
	static int Message_Queue_ID;
	static dev_data_t catch_data;
	static long int msg_type;
	pid_t curr_PID = getpid()*10;

	Message_Queue_ID = msgget((key_t)MSG_Q_KEY, 0666 );
    if (signo == SIGUSR1){
    	printf("\t\tSIGUSR1 Received\n");
    	
    	//printf("\t\tMessage Q ID: %i\tParent PID: %d\t",Message_Queue_ID,curr_PID);
    	//Get a message from the Message Queue
		msg_type = msgrcv(Message_Queue_ID, (void*)&catch_data, BUFFER_SIZE,curr_PID, 0 );
		if (msg_type == -1) {
					fprintf(stderr, "\t\tMessage receive from Child failed with error: %d\n", errno);
					kill(childPID,SIGKILL);
					exit(EXIT_FAILURE);
		}
		else if(catch_data.msg_type = curr_PID){
			//printf("\t\tData Received: PID: %d\n",catch_data.msg_data.pid);
			printf("\t\tReceived from Child --> Message type: %li\tDevice PID: %d\tDevice Type: %s\tDevice status: %d\tThreshold Value : %d\tCurrent Value: %d\n",
					catch_data.msg_type, 
					catch_data.msg_data.pid,
					catch_data.msg_data.devType,
					catch_data.msg_data.status,
					catch_data.msg_data.trshVal,
					catch_data.msg_data.currVal);

			//Sending FIFO to cloud
			strcpy(cloud_data.some_data, "Signal received");
			cloud_fifo_fd = open(CLOUD_FIFO_NAME, O_WRONLY);
			cloud_data.parent_pid = getpid();
			
			printf("Parent PID: %d sent %s,\n", cloud_data.parent_pid, cloud_data.some_data);
			write(cloud_fifo_fd, &cloud_data, sizeof(cloud_st));	
			
		}
		
	}
	else if (signo == SIGINT){
		kill(childPID,SIGINT);
		close(cloud_fifo_fd);
		unlink(CLOUD_FIFO_NAME);
	}
	else if (signo == SIGKILL){
		kill(childPID,SIGKILL);
		exit(0);
	}
}
void child_sig_handler(int signo)
{	
	static ack_data_t child_to_dev;
	
	if (signo == SIGINT){

		stop_devices = true;
	}
}




int main(int argc, char* argv[]){

	stop_devices = false;
	controller_running = true;
	static dev_data_t received_data;

	childPID = fork();


	if(childPID >= 0) // fork was successful
	{
		if(childPID == 0) // child process
		{
			static int msg_q_ID;
			static int msg_recv_status;
			static char dev_count_sen;
			static char dev_count_act;
			static int i;
			static pid_t parent_PID;

			static ack_data_t cntrl_to_dev;
			reg_sen registered_sen[3];
			reg_act registered_act[3];

			controller_running = true;
			dev_count_sen = 0;
			dev_count_act = 0;
			for(i=0;i<3;i++){
				registered_sen[i].msg_type=0;
			}
			struct sigaction action2;
			action2.sa_handler = child_sig_handler;
			sigemptyset(&action2.sa_mask);
			action2.sa_flags = 0;
			sigaction(SIGINT, &action2, 0);


			printf("\n Child Process: %d\n", getpid() );
			parent_PID = getppid();
			
			msg_q_ID = msgget((key_t)MSG_Q_KEY, 0666 | IPC_CREAT);

			printf("Message Q ID:  %d\n",msg_q_ID);	
			sleep(5);

			while(controller_running){
				if (stop_devices==true){
					for (i=0; i <3; i++){
						if(registered_act[i].msg_type!=0){
							cntrl_to_dev.msg_type = registered_act[i].msg_type * 10;
							strcpy(cntrl_to_dev.ack_msg,"STOP");
							printf("Sending To ACT--> Message Type: %li\tDevice message: %s\n", cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);
							if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, sizeof(ack_data_t), 0) == -1) {
					    		fprintf(stderr, "Message STOP message to Actuator failed with Error no: %d \n",errno);
					   			exit(EXIT_FAILURE);
							}
						}
						//Send STOP commands to Sensors
						if(registered_sen[i].msg_type!=0){
							cntrl_to_dev.msg_type = registered_sen[i].msg_type * 10;
							printf("Sending To SEN--> Message Type: %li\tDevice message: %s\n", cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);
							if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, sizeof(ack_data_t), 0) == -1) {
					    		fprintf(stderr, "Message STOP send to Sensor failed with Error no: %d \n",errno);
					   			exit(EXIT_FAILURE);
							}
						}
					}

					kill(parent_PID,SIGKILL);
					controller_running = false;
					break;
				}

				//Get a message from the Message Queue
				msg_recv_status = msgrcv(msg_q_ID, (void*)&received_data, BUFFER_SIZE, 0, 0 );
				if(msg_recv_status == -1) {
					fprintf(stderr, "Message receive Interrrupted with Error #: %d\n", errno);
					//exit(EXIT_FAILURE);
				}
				//Check to see if a sensor or actuator is sending a message
				else if (received_data.msg_type < (long int) 10000) {
					//If the check a device needed initialization
					if (received_data.msg_data.status == STATUS_INIT) {
						printf("Initializing Device in Database\n");
				   			
						//Sets up Acknowledge Data to be sent
						cntrl_to_dev.msg_type = (long int)received_data.msg_type * 10;
						strcpy(cntrl_to_dev.ack_msg,"ACK");
						
						printf("Sent to Registered Device: %li  %s\n",cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);

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
						printf("Received --> Message type: %li\tDevice PID: %d\tDevice Type: %s\tDevice status: %d\tThreshold Value : %d\tCurrent Value: %d\n",
								received_data.msg_type, 
								received_data.msg_data.pid,
								received_data.msg_data.devType,
								received_data.msg_data.status,
								received_data.msg_data.trshVal,
								received_data.msg_data.currVal);
						for (i=0; i <3; i++){
							if(registered_sen[i].pid==received_data.msg_data.pid){
								if(received_data.msg_data.currVal >= registered_sen[i].trshVal){

									//Sets up Acknowledge Data to be sent
									cntrl_to_dev.msg_type = registered_act[i].msg_type * 10;
									strcpy(cntrl_to_dev.ack_msg,"Activate");
									
									//Send Acknowledgement data to Actuator
									printf("Sending To ACT--> Message Type: %li\tDevice message: %s\n", cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);
									if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, sizeof(ack_data_t), 0) == -1) {
							    		fprintf(stderr, "Message send to Actuator failed with Error no: %d \n",errno);
							   			exit(EXIT_FAILURE);
									}
									//Send Signal to Parent
									kill(parent_PID, SIGUSR1);

									//Send Alarm Data to Parent through Message Queue
									received_data.msg_type = parent_PID*10;	
									printf("Sent to Parent --> Message type: %li\tDevice PID: %d\tDevice Type: %s\tDevice status: %d\tThreshold Value : %d\tCurrent Value: %d\n",
											received_data.msg_type, 
											received_data.msg_data.pid,
											received_data.msg_data.devType,
											received_data.msg_data.status,
											received_data.msg_data.trshVal,
											received_data.msg_data.currVal);
									if (msgsnd(msg_q_ID, (void *)&received_data, BUFFER_SIZE, 0) == -1) {
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
			printf("Exit program\n");
			return 0;
		
		}
		else //Parent process
		{
			struct sigaction action1;
			action1.sa_handler = parent_sig_handler;
			sigemptyset(&action1.sa_mask);
			action1.sa_flags = 0;
			sigaction(SIGUSR1, &action1, 0);
			sigaction(SIGINT, &action1, 0);
			sigaction(SIGKILL, &action1, 0);		

			while(1);
		}
	}
	else // fork failed
	{
		printf("\n Fork failed, quitting!!!!!!\n");
		return 1;
	}

	
	printf("Exit program\n");
	return 0;

}
