#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <tslib.h>

#include <input_my.h>
#include <sys/time.h>

static int fd;
static struct timeval last_time;
static struct timeval this_time;
static struct input_event remote_data;

static int remote_init(void)
{
    fd = open("/dev/input/event3", O_RDWR);
    return 0;
}

static int remote_exit(void)
{
    close(fd);
    return 0;
}

static int remote_getinputevent(pinputevent ptInputEvent)
{
    if (read(fd, &remote_data, sizeof(remote_data)) == sizeof(remote_data))
    {
        gettimeofday(&this_time, NULL);
        //如果这次输入和上次时间相差小于100mms，则忽略
        if ((this_time.tv_sec-last_time.tv_sec)*1000000+((this_time.tv_usec-last_time.tv_usec))<100000)
        {   
            last_time = this_time;
            return -1;
        }
        //间隔大于100mms,且值为1则上报
        else if (remote_data.value == 1)
        {
            ptInputEvent->iType = input_type_remote;
            ptInputEvent->tTime = this_time;
            ptInputEvent->event = remote_data;
            last_time = this_time;
            return 0;
        }
    }

    return -1;//接收有误，返回-1
}

static inputdevice remote = {
    .cname = "remote",
    .device_init = remote_init,
    .device_exit = remote_exit,
    .device_getinputevent = remote_getinputevent,
};

void remoteinput_register(void)
{
    input_device_register(&remote);
}

