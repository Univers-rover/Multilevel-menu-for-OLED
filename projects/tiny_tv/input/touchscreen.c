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

#include <input_elc.h>

static struct tsdev *ts;


static int touchscreen_init(void)
{
    ts = ts_setup(NULL,0);//自动获取触摸屏信息
    if (!ts) {
		perror("ts_setup");
		return -1;
	}

    return 0;
}

static int touchscreen_exit(void)
{
    ts_close(ts);
    return 0;
}

static int touchscreen_getinputevent(pinputevent ptInputEvent)
{
    int ret;
    struct ts_sample samp;
    ret = ts_read(ts, &samp, 1);
    if (ret < 0) {
        perror("ts_read");

        return -1;
    }

    ptInputEvent->iType      = input_type_touchscreen;
    ptInputEvent->ix         = samp.x;
    ptInputEvent->iy         = samp.y;
    ptInputEvent->iPressure   = samp.pressure;
    ptInputEvent->tTime      = samp.tv;

    return 0;
}

static inputdevice touchscreen = {
    .cname = "touchscreen",
    .device_init = touchscreen_init,
    .device_exit = touchscreen_exit,
    .device_getinputevent = touchscreen_getinputevent,
};

void touchinput_register(void)
{
    input_device_register(&touchscreen);
}

#if 0

int main(int argc, char **argv)
{
	inputevent event;
	int ret;
	
	touchscreen.device_init();

	while (1)
	{
		ret = touchscreen.device_getinputevent(&event);
		if (ret) {
			printf("GetInputEvent err!\n");
			return -1;
		}
		else
		{
			printf("Type      : %d\n", event.iType);
			printf("iX        : %d\n", event.ix);
			printf("iY        : %d\n", event.iy);
			printf("iPressure : %d\n", event.iPressure);
		}
	}
	return 0;
}

#endif
