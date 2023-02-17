#ifndef _INPUT_MY_H
#define _INPUT_MY_H

#ifndef NULL
#define NULL (void *)0
#endif

#include <linux/input.h>
#include <sys/time.h>

/*
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>
*/

#define input_type_remote 1
#define input_type_net    2

/* 输入事件结构体 */
typedef struct inputevent{
    struct timeval	tTime;
    struct input_event event;
    int iType;
    char mesg[1024];
}inputevent, *pinputevent;

typedef struct inputdevice{
    char *cname;
    int (*device_init)(void);
    int (*device_exit)(void);
    int (*device_getinputevent)(pinputevent ptinputevent);
    struct inputdevice *next;
}inputdevice, *pinputdevice;

/* 设备层使用 */
void input_device_register(pinputdevice ptinputdevice);

/* 管理层使用 */
void touchinput_register(void);
void netinput_register  (void);

/* 应用层使用 */
void input_init(void);
int  GetInputEvent(pinputevent ptinputevent);
int  GetInputEvent_unblock(pinputevent ptinputevent);
int  input_device_init(void);

#endif