/*	SYSC 4001 - Assignment 1
	NOTE: Read README file first!
	Authors:
		Monty Dhanani (100926543)
		Irusha Vidanamadura (100935300)
*/	
#include "msgProtocol.h"

int main()
{
	static int cloud_fifo_fd, parent_fifo_fd;
	static action_data_t catch_data;
	int read_res;

	//Creates FIFO
	//Reads message from parent process
	mkfifo(CLOUD_FIFO_NAME, 0777);
	cloud_fifo_fd = open(CLOUD_FIFO_NAME, O_RDONLY);

	//Prints message from parent process as long as it contains data
	do {
	read_res = read(cloud_fifo_fd, &catch_data, sizeof(action_data_t));
	printf("Receiving from parent --> Parent PID: %li\tDevice PID: %d\tDevice Type: %s\tDevice action: %s\tThreshold Value : %d\tCurrent Value: %d\n",
					catch_data.msg_type, 
					catch_data.action_data.pid,
					catch_data.action_data.devType,
					catch_data.action_data.action,
					catch_data.action_data.trshVal,
					catch_data.action_data.currVal);
	} while (read_res > 0);

	//Closes the FIFO connection and unlinks FIFO file
	close(cloud_fifo_fd);
	unlink(CLOUD_FIFO_NAME);
	printf("Exit program\n");
	return 0;
}
