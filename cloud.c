#include "msgProtocol.h"

int main()
{
    static int cloud_fifo_fd, parent_fifo_fd;
    static cloud_st cloud_data;
    int read_res;

    mkfifo(CLOUD_FIFO_NAME, 0777);
    cloud_fifo_fd = open(CLOUD_FIFO_NAME, O_RDONLY);

    do {
        read_res = read(cloud_fifo_fd, &cloud_data, sizeof(cloud_st));
	printf("Parent PID: %d, data: %s\n",cloud_data.parent_pid,cloud_data.some_data);
    } while (read_res > 0);
    close(cloud_fifo_fd);
    unlink(CLOUD_FIFO_NAME);
}
