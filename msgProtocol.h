#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/msg.h>

#define MSG_Q_KEY 4571

#define BUFFER_SIZE 512

#define PROCESS_SIGNAL 1

#define STATUS_INIT 1
#define STATUS_NORMAL 2
#define STATUS_ALARM 3



//Struct for Message structure
typedef struct msg_data_struct{
	pid_t pid;
	int status;
	int trshVal;
	int currVal;
	char devType[16];
}msg_data_t;

//Struct for Sensor/Actuator Data
typedef struct dev_data_struct{
	long int msg_type;
	msg_data_t msg_data;
} dev_data_t;

//Struct for ACK
typedef struct ack_data_struct{
	long int msg_type;
	char ack_msg[256];
} ack_data_t;
