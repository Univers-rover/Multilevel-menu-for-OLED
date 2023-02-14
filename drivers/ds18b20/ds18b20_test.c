
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <sys/ioctl.h>

#define CMD_TRIG  100

static int fd;

/*
 * ./button_test /dev/sr04
 *
 */
int main(int argc, char **argv)
{
	int val[2];
	struct pollfd fds[1];
	int timeout_ms = 5000;
	int ret;
	int	flags;

	int i;
	
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
		ret = 0;

		printf("I am goning to read tempreature: \n");
		ret = read(fd, val, 8);
		if ( ret > 0 )
		{
			printf("tempreature: %d.%d\n", val[0], val[1]);
			sleep(2);
		}
		else
		{
			printf("get tempreature err, err code is %d\n",ret);
			sleep(1);
		}
	}

	close(fd);
	
	return 0;
}


