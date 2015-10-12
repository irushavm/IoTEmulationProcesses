/*	SYSC 4001 - Assignment 1
	NOTE: Read README file first!
	Authors:
		Monty Dhanani (100926543)
		Irusha Vidanamadura (100935300)
*/	

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

static volatile bool stop_devices;
static volatile bool controller_running;

static int cloud_fifo_fd, parent_fifo_fd;

//Catches Signals Received for the Parent Process
void parent_sig_handler(int signo)
{	
	static int Message_Queue_ID;
	static action_data_t catch_data;
	static long int msg_type;
	pid_t curr_PID = getpid()*10;

	//Open the message queue
	Message_Queue_ID = msgget((key_t)MSG_Q_KEY, 0666 );
	//If User Defined Signal is received
	if (signo == SIGUSR1){

		printf("\t\tSIGUSR1 Received\n");
		//Get a message from the Message Queue
		msg_type = msgrcv(Message_Queue_ID, (void*)&catch_data, BUFFER_SIZE,curr_PID, 0 );
		if (msg_type == -1) {
					fprintf(stderr, "\t\tMessage receive from Child failed with error: %d\n", errno);
					kill(childPID,SIGKILL);
					exit(EXIT_FAILURE);		
					close(cloud_fifo_fd);
					unlink(CLOUD_FIFO_NAME);
		}
		else if(catch_data.msg_type = curr_PID){
			printf("\t\tRelaying from Child to cloud --> Message type: %li\tDevice PID: %d\tDevice Type: %s\tDevice action: %s\tThreshold Value : %d\tCurrent Value: %d\n",
					catch_data.msg_type, 
					catch_data.action_data.pid,
					catch_data.action_data.devType,
					catch_data.action_data.action,
					catch_data.action_data.trshVal,
					catch_data.action_data.currVal);
			//Opens FIFO File Connection	
			cloud_fifo_fd = open(CLOUD_FIFO_NAME, O_WRONLY);
			//Set the message type to the parent PID
			catch_data.msg_type = getpid();
			//Write data to Cloud FIFO
			write(cloud_fifo_fd, &catch_data, sizeof(action_data_t));	 
		
		}
	
	}
	else if (signo == SIGINT){
		kill(childPID,SIGINT);
		//Closes the FIFO connection and unlinks FIFO file
		close(cloud_fifo_fd);
		unlink(CLOUD_FIFO_NAME);
	}
	else if (signo == SIGKILL){
		kill(childPID,SIGKILL);
		exit(0);
	}
}

//Catches Signals sent by the Parent Process
void child_sig_handler(int signo)
{	
	if (signo == SIGINT){
		stop_devices = true;
	}
}




int main(int argc, char* argv[]){

	stop_devices = false;
	controller_running = true;

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

			static dev_data_t received_data;
			static ack_data_t cntrl_to_dev;
			static action_data_t child_to_parent;
			reg_sen registered_sen[MAX_ACT_SEN_AMOUNT];
			reg_act registered_act[MAX_ACT_SEN_AMOUNT];
			
			controller_running = true;
			dev_count_sen = 0;
			dev_count_act = 0;

			//Initializes all registered devices' message types to zero
			for(i=0;i<3;i++){
				registered_sen[i].msg_type=0;
				registered_act[i].msg_type=0;
			}
			
			//Set up signal handler for Interrupt calls from Parent
			struct sigaction action2;
			action2.sa_handler = child_sig_handler;
			sigemptyset(&action2.sa_mask);
			action2.sa_flags = 0;
			sigaction(SIGINT, &action2, 0);


			printf("\nChild Process: %d\n", getpid() );
			parent_PID = getppid();
			
			//Create Message Queue
			msg_q_ID = msgget((key_t)MSG_Q_KEY, 0666 | IPC_CREAT);

			printf("Message Q ID:  %d\n",msg_q_ID);	

			//Gives some time for devices to initialize
			sleep(5);

			while(controller_running){
				//Check if a command to stop all devices has been given from the Parent Process
				if (stop_devices==true){
					for (i=0; i <3; i++){
						//Send STOP commands to Actuator
						if(registered_act[i].msg_type!=0){
							cntrl_to_dev.msg_type = registered_act[i].msg_type * 10;
							strcpy(cntrl_to_dev.ack_msg,"STOP");
							printf("Sending To ACT--> Message Type: %li\tDevice message: %s\n", cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);
							if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, sizeof(ack_data_t), 0) == -1) {
					    		fprintf(stderr, "Message STOP message to Actuator failed with Error no: %d \n",errno);
					   			exit(EXIT_FAILURE);
							}
						}
						//Send STOP commands to Sensor
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
				else if (received_data.msg_type < (long int) MAX_PC_PROCESS) {

					//If the check a device needed Registering
					if (received_data.msg_data.status == STATUS_INIT) {
						printf("---------------\nInitializing Device in Database\n");
				   			
						//Set up Acknowledge Data to be sent
						cntrl_to_dev.msg_type = (long int)received_data.msg_type * 10;
						strcpy(cntrl_to_dev.ack_msg,"ACK");
						printf("Sending To Registered Device --> %li  %s\n---------------\n",cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);

						//Send Acknowledgement data
						if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, BUFFER_SIZE, 0) == -1) {
				    		fprintf(stderr, "ERROR: Failed to send Ackowledge to Device. Error # %d \n",errno);
				   			exit(EXIT_FAILURE);
						}
						//If a sensor needs Registering
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
									//Send Alarm Data to Parent through Message Queue
									child_to_parent.msg_type = parent_PID*10;
									child_to_parent.action_data.pid = received_data.msg_data.pid;
									strcpy(child_to_parent.action_data.devType,received_data.msg_data.devType);
									strcpy(child_to_parent.action_data.action,"Activate ");
									strcat(child_to_parent.action_data.action,registered_act[i].devType);
									child_to_parent.action_data.trshVal=registered_sen[i].trshVal;
									child_to_parent.action_data.currVal = received_data.msg_data.currVal;
									
									printf("Sending to Parent --> Message type: %li\tDevice PID: %d\tDevice Type: %s\tDevice Action: %s\tThreshold Value : %d\tCurrent Value: %d\n",
											child_to_parent.msg_type, 
											child_to_parent.action_data.pid,
											child_to_parent.action_data.devType,
											child_to_parent.action_data.action,
											child_to_parent.action_data.trshVal,
											child_to_parent.action_data.currVal);
									if (msgsnd(msg_q_ID, (void *)&child_to_parent, BUFFER_SIZE, 0) == -1) {
							    		fprintf(stderr, "Message send to Parent Process failed with Error no: %d \n",errno);
							   			exit(EXIT_FAILURE);
									}	


									//Sets up Acknowledge Data to be sent
									cntrl_to_dev.msg_type = registered_act[i].msg_type * 10;
									strcpy(cntrl_to_dev.ack_msg,"Activate");
									
									//Send action data to Actuator
									printf("Sending To ACT--> Message Type: %li\tDevice message: %s\n", cntrl_to_dev.msg_type,cntrl_to_dev.ack_msg);
									if (msgsnd(msg_q_ID, (void *)&cntrl_to_dev, sizeof(ack_data_t), 0) == -1) {
							    		fprintf(stderr, "Message send to Actuator failed with Error no: %d \n",errno);
							   			exit(EXIT_FAILURE);
									}
									if(msgrcv(msg_q_ID, (void*)&cntrl_to_dev, BUFFER_SIZE, registered_act[i].msg_type, 0 ) == -1) {
										fprintf(stderr, "Ack message not received with Error #: %d\n", errno);
										exit(EXIT_FAILURE);
									}
									if (strncmp(cntrl_to_dev.ack_msg, "ACK", 3) == 0) {
										printf("ACK Received from ACT\n");
									}else{
										fprintf(stderr, "Message does not contain \"ACK\" with Error #: %d\n", errno);	
										exit(EXIT_FAILURE);
									}
									
									//Send Signal to Parent
									kill(parent_PID, SIGUSR1);									
								
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
			//Set up signal handler for Child-Parent communication and Keyboard Interrupts
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
		printf("\nERROR: Fork failed.\n");
		return 1;
	}

	
	printf("Exit program\n");
	return 0;

}
