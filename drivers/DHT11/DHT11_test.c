
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>

static int fd;

/*
 * ./button_test /dev/motor -100  1
 *
 */
int main(int argc, char **argv)
{
	unsigned char val[4];
	int ret;
	
	/* 1. 判断参数 */
	if (argc != 2) 
	{
		printf("Usage: %s <dev>\n", argv[0]);
		return -1;
	}


	/* 2. 打开文件 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1)
	{
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	while (1)
	{
		ret = read(fd, val, sizeof(val));
		while (1)
		{
			if (read(fd, &val, 4) == 4)
			{
				printf("Humidity = %d.%d\n\r", val[0], val[1]);
				printf("Temperature = %d.%d\n\r", val[2], val[3]);
				break;
			}
		}

		sleep(2);
		//printf("reading complete\n");
	}

	close(fd);
}


